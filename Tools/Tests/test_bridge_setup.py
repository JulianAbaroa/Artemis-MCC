"""
test_bridge_setup.py
Tests of request_setup of the Python bridge client (LS.2): the fields and the mask are written before the counter is raised, the call
returns the SetupStatus (result, round and reason) after Artemis echoed the counter, the roster is written as the bytes of the layout and
read back, and a dict or a MatchSetup is one setup.
The real Bridge opens Windows shared memory, so the object is built without opening it, on plain numpy records of the generated layout,
and kernel32 is stood in for the import.
"""
import ctypes
import importlib
import os
import sys
import threading
import time
from unittest import mock

import numpy as np
import pytest

from tdump_writer import Repo

AGENT = os.path.join(Repo(), "Agent")


@pytest.fixture
def modules(monkeypatch):
    """(artemis.bridge, artemis.bridge_layout) imported with a stand-in for kernel32."""
    monkeypatch.syspath_prepend(AGENT)
    monkeypatch.setattr(ctypes, "WinDLL", lambda *args, **kwargs: mock.MagicMock(), raising=False)
    for name in [n for n in sys.modules if n == "artemis" or n.startswith("artemis.")]:
        monkeypatch.delitem(sys.modules, name)
    bridge = importlib.import_module("artemis.bridge")
    layout = importlib.import_module("artemis.bridge_layout")
    yield bridge, layout
    for name in [n for n in sys.modules if n == "artemis" or n.startswith("artemis.")]:
        sys.modules.pop(name, None)


@pytest.fixture
def client(modules):
    """A Bridge on a control block in memory, with nobody answering."""
    bridge_module, layout = modules
    control = np.zeros((), dtype=layout.BridgeControl)
    bridge = object.__new__(bridge_module.Bridge)
    bridge.artemis, bridge.client = control["Artemis"], control["Client"]
    return bridge, layout




def Echo(bridge, sequence, result, delay=0.05, round_index=-1, reason=b""):
    """Artemis taking the request after a delay: the answer first and the sequence last."""
    def Run():
        time.sleep(delay)
        bridge.artemis["SetupResult"] = result
        bridge.artemis["SetupRound"] = round_index
        bridge.artemis["SetupReason"][:] = np.frombuffer(reason.ljust(64, b"\0"), dtype=np.uint8)
        bridge.artemis["SetupSequence"] = sequence
    thread = threading.Thread(target=Run)
    thread.start()
    return thread


def Text(field):
    return bytes(np.asarray(field, dtype=np.uint8)).split(b"\0", 1)[0].decode()


def test_a_setup_writes_its_fields_and_the_mask_before_the_counter(client, modules):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgeSetupResult.Pending)
    status = bridge.request_setup(preset="artemis_ctf_fast", map="reach_bus", roster={"0": 3, "1": 4, "auto": 2})
    thread.join()

    assert status.result == layout.BridgeSetupResult.Pending
    assert status.round == -1 and status.reason == ""
    client_block = bridge.client
    assert int(client_block["RequestSetup"]) == 1
    assert int(client_block["RequestSetupFields"]) == layout.SETUP_PRESET | layout.SETUP_MAP | layout.SETUP_ROSTER
    assert Text(client_block["RequestSetupPreset"]) == "artemis_ctf_fast"
    assert Text(client_block["RequestSetupMap"]) == "reach_bus"
    assert [int(count) for count in client_block["RequestSetupRoster"]["Teams"]] == [3, 4, 0, 0, 0, 0, 0, 0]
    assert int(client_block["RequestSetupRoster"]["Auto"]) == 2


def test_only_the_fields_given_are_in_the_mask(client):
    bridge, layout = client
    bridge.request_setup(roster=[0, 5], wait=False)
    assert int(bridge.client["RequestSetupFields"]) == layout.SETUP_ROSTER
    assert [int(count) for count in bridge.client["RequestSetupRoster"]["Teams"]][:2] == [0, 5]

    # An empty name with its bit set asks for the base values, which is not the same as no field.
    bridge.request_setup(preset="", wait=False)
    assert int(bridge.client["RequestSetupFields"]) == layout.SETUP_PRESET
    assert not np.asarray(bridge.client["RequestSetupPreset"]).any()
    assert int(bridge.client["RequestSetup"]) == 2


def test_without_wait_and_without_an_echo_nothing_is_returned(client):
    bridge, _ = client
    assert bridge.request_setup(preset="a", wait=False) is None
    assert bridge.request_setup(preset="a", timeout=0.1) is None
    assert int(bridge.client["RequestSetup"]) == 2


def test_the_status_carries_the_round_and_the_reason(client):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgeSetupResult.Applied, round_index=2, reason=b"partial: 2 of 4 bots")
    status = bridge.request_setup(roster={"0": 4})
    thread.join()
    assert tuple(status) == (layout.BridgeSetupResult.Applied, 2, "partial: 2 of 4 bots")

    bridge.artemis["SetupResult"] = 200
    assert bridge.setup_status.result == layout.BridgeSetupResult.Unknown


def test_names_that_do_not_fit_are_refused_before_anything_is_written(client):
    bridge, layout = client
    with pytest.raises(ValueError):
        bridge.request_setup(preset="p" * layout.PRESET_NAME_SIZE)
    with pytest.raises(ValueError):
        bridge.request_setup(map="m" * (layout.MAP_NAME_SIZE + 1))
    with pytest.raises(ValueError):
        bridge.request_setup(roster={"8": 1})
    with pytest.raises(ValueError):
        bridge.request_setup(roster={"0": 256})
    assert int(bridge.client["RequestSetup"]) == 0
    assert int(bridge.client["RequestSetupFields"]) == 0


def test_the_shortcuts_are_a_setup_with_one_field(client):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgeSetupResult.Pending)
    assert bridge.request_preset("fast") == layout.BridgeSetupResult.Pending
    thread.join()
    assert int(bridge.client["RequestSetupFields"]) == layout.SETUP_PRESET

    thread = Echo(bridge, 2, layout.BridgeSetupResult.AtLoad)
    assert bridge.request_map("reach_bus") == layout.BridgeSetupResult.AtLoad
    thread.join()
    assert int(bridge.client["RequestSetupFields"]) == layout.SETUP_MAP


def test_the_active_setup_is_read_from_the_control(client):
    bridge, layout = client
    assert bridge.active_roster is None

    bridge.artemis["ActivePreset"][:] = np.frombuffer(b"artemis_ctf_base".ljust(layout.PRESET_NAME_SIZE, b"\0"), dtype=np.uint8)
    bridge.artemis["ActiveMap"][:] = np.frombuffer(b"reach_bus".ljust(layout.MAP_NAME_SIZE, b"\0"), dtype=np.uint8)
    bridge.artemis["ActiveRoster"]["Teams"][1] = 5
    bridge.artemis["ActiveRoster"]["Auto"] = 1
    assert bridge.active_roster is None  # the flag says whether it has one
    bridge.artemis["ActiveHasRoster"] = 1

    assert bridge.active_preset == "artemis_ctf_base"
    assert bridge.active_map == "reach_bus"
    assert bridge.active_roster == {"1": 5, "auto": 1}


def test_wait_setup_waits_while_it_is_pending(client):
    bridge, layout = client
    bridge.artemis["SetupResult"] = layout.BridgeSetupResult.Pending
    thread = Echo(bridge, 1, layout.BridgeSetupResult.Applied, delay=0.1, round_index=1)
    assert bridge.wait_setup(2.0) == (layout.BridgeSetupResult.Applied, 1, "")
    thread.join()

    bridge.artemis["SetupResult"] = layout.BridgeSetupResult.Pending
    assert bridge.wait_setup(0.1).result == layout.BridgeSetupResult.Pending


def test_a_client_that_starts_takes_the_sequence_of_artemis_as_its_counter(client):
    # The init block of Bridge copies SetupSequence into RequestSetup, so a counter left by another client is not a request.
    bridge, layout = client
    source = open(os.path.join(AGENT, "artemis", "bridge.py"), encoding="utf-8").read()
    assert 'self.client["RequestSetup"] = int(self.artemis["SetupSequence"])' in source


def test_a_dict_and_a_match_setup_are_the_same_setup(modules):
    bridge_module, _ = modules
    from_dict = bridge_module.MatchSetup.of({"preset": "p", "roster": {"0": 1}})
    assert from_dict == bridge_module.MatchSetup("p", None, {"0": 1})
    assert bridge_module.MatchSetup.of(from_dict) is from_dict
    assert bridge_module.MatchSetup.of(None) == bridge_module.MatchSetup()
    with pytest.raises(ValueError):
        bridge_module.MatchSetup.of({"presett": "p"})


def test_the_roster_is_the_teams_then_auto(modules):
    bridge_module, _ = modules
    array = bridge_module.roster_to_array({0: 7, "1": 8, "auto": 2})
    assert list(array) == [7, 8, 0, 0, 0, 0, 0, 0, 2]
    assert bridge_module.roster_to_dict(array[:8], array[8]) == {"0": 7, "1": 8, "auto": 2}
