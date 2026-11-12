"""
test_bridge_map.py
Tests of request_map, the shortcut of request_setup(map=...) of the Python bridge client (Q98, D266, LS.2): the name is written before the counter is raised, the call
returns the result after Artemis echoed the counter, a name of the full 64 bytes fits without an end zero, and the control block carries
the one setup block.
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


def Echo(bridge, sequence, result, delay=0.05):
    """Artemis taking the request after a delay: the sequence and the result."""
    def Run():
        time.sleep(delay)
        bridge.artemis["SetupResult"] = result
        bridge.artemis["SetupSequence"] = sequence
    thread = threading.Thread(target=Run)
    thread.start()
    return thread


def test_the_layout_has_the_one_setup_block(modules):
    _, layout = modules
    assert layout.MAP_NAME_SIZE == 64
    assert layout.SETUP_PRESET | layout.SETUP_MAP | layout.SETUP_ROSTER == 7
    for name in ("SetupSequence", "SetupResult", "SetupRound", "SetupReason", "ActiveMap", "ActivePreset", "ActiveRoster"):
        assert name in layout.BridgeArtemisControl.names
    for name in ("RequestSetup", "RequestSetupFields", "RequestSetupPreset", "RequestSetupMap", "RequestSetupRoster"):
        assert name in layout.BridgeClientControl.names
    # The counter of the request and the echo are the last of their parts: the fields come before them.
    assert layout.BridgeArtemisControl.names[-1] == "SetupSequence"
    assert layout.OFFSETS["Control.Client.RequestSetup"] > layout.OFFSETS["Control.Client.RequestSetupRoster"]


def test_the_name_is_zero_filled_and_the_counter_raises_after_it(client):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgeSetupResult.Pending)
    result = bridge.request_map("Reach Bus Mountain Pass Reach Bus CTF")
    thread.join()

    assert result == layout.BridgeSetupResult.Pending
    assert int(bridge.client["RequestSetup"]) == 1
    assert int(bridge.client["RequestSetupFields"]) == layout.SETUP_MAP
    raw = bytes(np.asarray(bridge.client["RequestSetupMap"], dtype=np.uint8))
    assert len(raw) == layout.MAP_NAME_SIZE
    assert raw == b"Reach Bus Mountain Pass Reach Bus CTF".ljust(layout.MAP_NAME_SIZE, b"\0")


def test_without_wait_the_request_is_raised_and_nothing_is_waited_for(client):
    bridge, _ = client
    assert bridge.request_map("reach_bus", wait=False) is None
    assert int(bridge.client["RequestSetup"]) == 1


def test_a_shorter_name_leaves_nothing_of_the_previous_one(client):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgeSetupResult.Pending)
    bridge.request_map("reach_bus")
    thread.join()
    thread = Echo(bridge, 2, layout.BridgeSetupResult.Pending)
    bridge.request_map("")
    thread.join()

    assert int(bridge.client["RequestSetup"]) == 2
    assert not np.asarray(bridge.client["RequestSetupMap"]).any()


def test_a_name_of_sixty_four_characters_fits_and_one_more_is_refused(client):
    bridge, layout = client
    name = "m" * layout.MAP_NAME_SIZE
    assert bridge.request_map(name, wait=False) is None
    assert bytes(np.asarray(bridge.client["RequestSetupMap"], dtype=np.uint8)) == name.encode()
    with pytest.raises(ValueError):
        bridge.request_map(name + "m")


def test_no_echo_gives_none(client):
    bridge, _ = client
    assert bridge.request_map("a", timeout=0.1) is None


def test_the_result_and_the_active_map_are_read_from_the_control(client):
    bridge, layout = client
    assert bridge.setup_result == layout.BridgeSetupResult.NoResult
    assert bridge.active_map == ""

    bridge.artemis["SetupResult"] = layout.BridgeSetupResult.Rejected
    bridge.artemis["ActiveMap"][:] = np.frombuffer(b"reach_bus".ljust(layout.MAP_NAME_SIZE, b"\0"), dtype=np.uint8)
    assert bridge.setup_result == layout.BridgeSetupResult.Rejected
    assert bridge.active_map == "reach_bus"
    assert bridge.active_preset == ""

    bridge.artemis["SetupResult"] = 200
    assert bridge.setup_result == layout.BridgeSetupResult.Unknown


def test_wait_setup_waits_while_it_is_pending(client):
    bridge, layout = client
    bridge.artemis["SetupResult"] = layout.BridgeSetupResult.Pending
    thread = Echo(bridge, 1, layout.BridgeSetupResult.Applied, delay=0.1)
    assert bridge.wait_setup(2.0).result == layout.BridgeSetupResult.Applied
    thread.join()

    bridge.artemis["SetupResult"] = layout.BridgeSetupResult.Pending
    assert bridge.wait_setup(0.1).result == layout.BridgeSetupResult.Pending
