"""
test_bridge_preset.py
Tests of the gametype preset request of the Python bridge client (Q97): the name is written before the counter is raised, the call
returns the result after Artemis echoed the counter, and the reset of the CTF environment requests the preset and ends the round.
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
        bridge.artemis["PresetResult"] = result
        bridge.artemis["PresetSequence"] = sequence
    thread = threading.Thread(target=Run)
    thread.start()
    return thread


def test_the_name_is_zero_filled_and_the_counter_raises_after_it(client):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgePresetResult.Pending)
    result = bridge.request_preset("artemis_ctf_fast")
    thread.join()

    assert result == layout.BridgePresetResult.Pending
    assert int(bridge.client["RequestPreset"]) == 1
    raw = bytes(np.asarray(bridge.client["RequestPresetName"], dtype=np.uint8))
    assert len(raw) == layout.PRESET_NAME_SIZE
    assert raw == b"artemis_ctf_fast".ljust(layout.PRESET_NAME_SIZE, b"\0")


def test_without_wait_the_request_is_raised_and_nothing_is_waited_for(client):
    bridge, _ = client
    assert bridge.request_preset("artemis_ctf_fast", wait=False) is None
    assert int(bridge.client["RequestPreset"]) == 1


def test_a_shorter_name_leaves_nothing_of_the_previous_one(client):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgePresetResult.Pending)
    bridge.request_preset("artemis_ctf_fast")
    thread.join()
    thread = Echo(bridge, 2, layout.BridgePresetResult.Pending)
    bridge.request_preset("")
    thread.join()

    assert int(bridge.client["RequestPreset"]) == 2
    assert not np.asarray(bridge.client["RequestPresetName"]).any()


def test_no_echo_gives_none_and_a_name_that_does_not_fit_is_refused(client):
    bridge, layout = client
    assert bridge.request_preset("a", timeout=0.1) is None
    with pytest.raises(ValueError):
        bridge.request_preset("x" * layout.PRESET_NAME_SIZE)


def test_the_result_and_the_active_preset_are_read_from_the_control(client):
    bridge, layout = client
    assert bridge.preset_result == layout.BridgePresetResult.NoResult
    assert bridge.active_preset == ""

    bridge.artemis["PresetResult"] = layout.BridgePresetResult.Rejected
    bridge.artemis["ActivePreset"][:] = np.frombuffer(b"artemis_ctf_base".ljust(layout.PRESET_NAME_SIZE, b"\0"), dtype=np.uint8)
    assert bridge.preset_result == layout.BridgePresetResult.Rejected
    assert bridge.active_preset == "artemis_ctf_base"

    bridge.artemis["PresetResult"] = 200
    assert bridge.preset_result == layout.BridgePresetResult.Unknown


def test_wait_preset_waits_while_it_is_pending(client):
    bridge, layout = client
    bridge.artemis["PresetResult"] = layout.BridgePresetResult.Pending
    thread = Echo(bridge, 1, layout.BridgePresetResult.Applied, delay=0.1)
    assert bridge.wait_preset(2.0) == layout.BridgePresetResult.Applied
    thread.join()

    bridge.artemis["PresetResult"] = layout.BridgePresetResult.Pending
    assert bridge.wait_preset(0.1) == layout.BridgePresetResult.Pending
