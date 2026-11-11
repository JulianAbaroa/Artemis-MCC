"""
test_bridge_map.py
Tests of the map variant request of the Python bridge client (Q98, D266): the name is written before the counter is raised, the call
returns the result after Artemis echoed the counter, a name of the full 64 bytes fits without an end zero, and the control block carries
the map variant fields next to the preset ones without mixing them.
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
        bridge.artemis["MapResult"] = result
        bridge.artemis["MapSequence"] = sequence
    thread = threading.Thread(target=Run)
    thread.start()
    return thread


def test_the_layout_has_the_map_fields_after_the_preset_ones(modules):
    _, layout = modules
    assert layout.MAP_NAME_SIZE == 64
    for name in ("MapSequence", "MapResult", "ActiveMap"):
        assert name in layout.BridgeArtemisControl.names
    for name in ("RequestMap", "RequestMapName"):
        assert name in layout.BridgeClientControl.names
    assert layout.OFFSETS["Control.Artemis.MapSequence"] > layout.OFFSETS["Control.Artemis.ActivePreset"] + layout.PRESET_NAME_SIZE - 1
    assert layout.OFFSETS["Control.Client.RequestMap"] > layout.OFFSETS["Control.Client.RequestPresetName"]


def test_the_name_is_zero_filled_and_the_counter_raises_after_it(client):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgePresetResult.Pending)
    result = bridge.request_map("Reach Bus Mountain Pass Reach Bus CTF")
    thread.join()

    assert result == layout.BridgePresetResult.Pending
    assert int(bridge.client["RequestMap"]) == 1
    assert int(bridge.client["RequestPreset"]) == 0
    raw = bytes(np.asarray(bridge.client["RequestMapName"], dtype=np.uint8))
    assert len(raw) == layout.MAP_NAME_SIZE
    assert raw == b"Reach Bus Mountain Pass Reach Bus CTF".ljust(layout.MAP_NAME_SIZE, b"\0")


def test_without_wait_the_request_is_raised_and_nothing_is_waited_for(client):
    bridge, _ = client
    assert bridge.request_map("reach_bus", wait=False) is None
    assert int(bridge.client["RequestMap"]) == 1


def test_a_shorter_name_leaves_nothing_of_the_previous_one(client):
    bridge, layout = client
    thread = Echo(bridge, 1, layout.BridgePresetResult.Pending)
    bridge.request_map("reach_bus")
    thread.join()
    thread = Echo(bridge, 2, layout.BridgePresetResult.Pending)
    bridge.request_map("")
    thread.join()

    assert int(bridge.client["RequestMap"]) == 2
    assert not np.asarray(bridge.client["RequestMapName"]).any()


def test_a_name_of_sixty_four_characters_fits_and_one_more_is_refused(client):
    bridge, layout = client
    name = "m" * layout.MAP_NAME_SIZE
    assert bridge.request_map(name, wait=False) is None
    assert bytes(np.asarray(bridge.client["RequestMapName"], dtype=np.uint8)) == name.encode()
    with pytest.raises(ValueError):
        bridge.request_map(name + "m")


def test_no_echo_gives_none(client):
    bridge, _ = client
    assert bridge.request_map("a", timeout=0.1) is None


def test_the_result_and_the_active_map_are_read_from_the_control(client):
    bridge, layout = client
    assert bridge.map_result == layout.BridgePresetResult.NoResult
    assert bridge.active_map == ""

    bridge.artemis["MapResult"] = layout.BridgePresetResult.Rejected
    bridge.artemis["ActiveMap"][:] = np.frombuffer(b"reach_bus".ljust(layout.MAP_NAME_SIZE, b"\0"), dtype=np.uint8)
    assert bridge.map_result == layout.BridgePresetResult.Rejected
    assert bridge.active_map == "reach_bus"
    assert bridge.preset_result == layout.BridgePresetResult.NoResult
    assert bridge.active_preset == ""

    bridge.artemis["MapResult"] = 200
    assert bridge.map_result == layout.BridgePresetResult.Unknown


def test_wait_map_waits_while_it_is_pending(client):
    bridge, layout = client
    bridge.artemis["MapResult"] = layout.BridgePresetResult.Pending
    thread = Echo(bridge, 1, layout.BridgePresetResult.Applied, delay=0.1)
    assert bridge.wait_map(2.0) == layout.BridgePresetResult.Applied
    thread.join()

    bridge.artemis["MapResult"] = layout.BridgePresetResult.Pending
    assert bridge.wait_map(0.1) == layout.BridgePresetResult.Pending
