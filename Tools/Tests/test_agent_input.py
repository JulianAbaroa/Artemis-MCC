"""
test_agent_input.py
Tests of the Python side of the bridge v2 (VW.9c.3 D3, D365): the layout of the action and the header, write_actions as the bytes of the
BridgeAction, the imitation block of a read tick, the four states of a control, and the input profiles of CtfEnv (MultiDiscrete ->
Action). Like the other bridge tests, the Bridge is built without opening Windows shared memory and kernel32 is stood in for the import.
"""
import ctypes
import importlib
import struct
from unittest import mock

import numpy as np
import pytest

from test_bridge_setup import modules  # noqa: F401  (fixture: artemis.bridge and artemis.bridge_layout with a stand-in for kernel32)


@pytest.fixture
def client(modules):  # noqa: F811
    """A Bridge with the Actions of a region in memory, and the CtfEnv module."""
    bridge_module, layout = modules
    bridge = object.__new__(bridge_module.Bridge)
    bridge.actions = np.zeros(4, dtype=layout.BridgeAction)
    bridge._action_sequence = ctypes.c_uint64(0)
    bridge._action_event = mock.MagicMock()
    ctf = importlib.import_module("artemis.env.ctf")
    return bridge, bridge_module, layout, ctf


def test_the_layout_is_the_v2_one(modules):  # noqa: F811
    _, layout = modules
    assert layout.BridgeAgentInput.itemsize == 24
    assert layout.BridgeAgentInput.names == ("MoveForward", "MoveLeft", "YawDelta", "PitchDelta", "Level", "Edge")
    assert layout.BridgeAction.names == ("PlayerHandle", "DecisionTick", "Input") and layout.BridgeAction.itemsize == 40
    assert layout.BridgeTickHeader.names[-2:] == ("PossessedPlayer", "PossessedInput")
    for gone in ("Buttons", "Lanes", "Yaw", "Pitch", "ThrottleForward", "ThrottleLeft", "Requests"):
        assert gone not in layout.BridgeAction.names


def test_an_action_is_written_as_the_bytes_of_the_bridge_action(client):
    bridge, module, layout, _ = client
    action = module.Action(handle=0x1234, forward=1.0, left=-1.0, yaw_delta=0.25, pitch_delta=-0.5, level=0b101, edge=0b110)
    bridge.write_actions(40, {2: action})

    raw = bridge.actions[2:3].tobytes()
    assert struct.unpack("<I4xQffffII", raw) == (0x1234, 40, 1.0, -1.0, 0.25, -0.5, 0b101, 0b110)
    assert bridge._action_sequence.value == 40
    assert not bridge.actions[0].tobytes().strip(b"\0")  # players left out are not touched


def test_the_four_states_of_a_control_are_level_and_edge(client):
    _, module, _, _ = client
    states = {"Jump": 0, "Crouch": 1, "Fire": 2, "Zoom": 3, "Reload": 2}
    level, edge = module.PackControls(states)
    # nothing (0,0), tap (0,1), press and hold (1,1), still held (1,0)
    assert level == (1 << 2) | (1 << 3) | (1 << 12)
    assert edge == (1 << 1) | (1 << 2) | (1 << 12)
    assert module.PackControls({}) == (0, 0)
    assert module.CONTROLS.index("Jump") == 0 and len(module.CONTROLS) == 13


def test_a_read_tick_carries_the_imitation_block(client):
    _, module, layout, _ = client
    data = np.zeros((), dtype=layout.BridgeTick)
    data["Header"]["PossessedPlayer"] = layout.NO_HANDLE
    assert module.PublishedTick(data, 0).possessed_player == layout.NO_HANDLE

    data["Header"]["PossessedPlayer"] = 0x10005
    data["Header"]["PossessedInput"]["YawDelta"] = 0.5
    data["Header"]["PossessedInput"]["Level"] = 3
    tick = module.PublishedTick(data, 0)
    assert tick.possessed_player == 0x10005
    assert float(tick.possessed_input["YawDelta"]) == 0.5 and int(tick.possessed_input["Level"]) == 3


def test_the_profiles_give_the_branches(client):
    _, _, _, ctf = client
    assert ctf.ActionNvec("stage0") == (3, 3, 9, 5, 4, 4, 4)
    assert ctf.ActionNvec("keyboard") == (3, 3, 9, 5) + (4,) * 13
    assert ctf.CtfConfig().profile == "stage0"


def test_stage0_maps_move_look_jump_and_crouch_and_nothing_else(client):
    _, module, _, ctf = client
    branches = np.array([2, 0, 8, 0, 2, 1])  # forward, left back, full yaw right, full pitch down, jump held, crouch tap
    action = ctf.MakeAction(0x10001, branches, "stage0", per_decision=0.4)
    assert (action.forward, action.left) == (1.0, -1.0)
    assert action.yaw_delta == pytest.approx(0.4) and action.pitch_delta == pytest.approx(-0.5 * 0.4)
    assert action.level == 0b01 and action.edge == 0b11  # Jump (1,1), Crouch (0,1)


def test_the_keyboard_profile_drives_the_13_controls(client):
    _, module, _, ctf = client
    branches = [1, 1, 4, 2] + [(i % 4) for i in range(13)]
    action = ctf.MakeAction(1, branches, "keyboard", per_decision=1.0)
    assert action.yaw_delta == 0.0 and action.pitch_delta == 0.0 and action.forward == 0.0
    for i in range(13):
        level, edge = module.CONTROL_STATES[i % 4]
        assert (action.level >> i) & 1 == level and (action.edge >> i) & 1 == edge
    assert action.level >> 13 == 0 and action.edge >> 13 == 0


def test_an_unknown_profile_is_refused(client):
    _, _, _, ctf = client
    with pytest.raises(ValueError, match="profile"):
        ctf.CtfEnv(None, ctf.CtfConfig(profile="gamepad"))
    env = ctf.CtfEnv(None, ctf.CtfConfig(profile="keyboard"))
    assert env.nvec == ctf.ActionNvec("keyboard") and env.obs_size == ctf.ACTION_AT + sum(env.nvec)
