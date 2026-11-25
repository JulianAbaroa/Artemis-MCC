"""
test_bridge_view.py
P.3 B (D374): the read of a tick without copying the Views (Bridge.read_tick_view) and its validation with the seqlock (is_unchanged), over
plain numpy records of the generated layout (the real Bridge opens Windows shared memory, so it is built without opening it, as in
test_bridge_setup.py). The Views are a numpy view on the buffer, the small blocks are copies, the omniscient blocks come back empty.
"""
import numpy as np
import pytest

from test_bridge_setup import modules  # noqa: F401  (fixture: artemis.bridge and artemis.bridge_layout with a stand-in for kernel32)


@pytest.fixture
def region(modules):  # noqa: F811
    """(bridge, layout, buffers): a Bridge on a bytearray with tick 0 published (even Sequence 2) with 2 players, 2 Views and 3 entities."""
    bridge_module, layout = modules
    bridge = object.__new__(bridge_module.Bridge)
    bridge.region = np.ndarray((), dtype=layout.BridgeRegion, buffer=bytearray(layout.REGION_SIZE))
    bridge.artemis = bridge.region["Control"]["Artemis"]
    bridge.buffers = bridge.region["Ticks"]
    bridge.tears, bridge.last_copy_ns, bridge.last_published_ticks, bridge._tick_event = 0, 0, 0, 0
    bridge.artemis["Magic"] = layout.MAGIC
    bridge.artemis["ActiveBuffer"] = 0
    tick = bridge.buffers[0]
    header = tick["Header"]
    header["PlayerCount"], header["ViewCount"], header["EntityCount"], header["ActionTick"], header["MatchId"] = 2, 2, 3, 12, 5
    tick["Players"]["Handle"][:2] = (7, 8)
    tick["Entities"]["Handle"][:3] = (1, 2, 3)
    tick["Match"]["GameTick"] = 100
    tick["Views"]["Head"]["Side"][:2] = (1, 1)
    tick["Sequence"] = tick["SequenceEnd"] = 2
    return bridge, bridge_module, layout


def test_the_views_are_a_view_of_the_buffer_and_the_small_blocks_are_copies(region):
    bridge, _, _ = region
    published = bridge.read_tick_view()
    assert published.action_tick == 12 and published.match_id == 5 and published.buffer_index == 0 and published.sequence == 2
    assert len(published.players) == 2 and len(published.views) == 2
    assert np.shares_memory(published.views, bridge.buffers["Views"][0])
    assert not np.shares_memory(published.players, bridge.buffers["Players"][0])
    assert not np.shares_memory(published.header, bridge.buffers[0]["Header"])
    # A change of the buffer is seen by the views and not by the copies.
    tick = bridge.buffers[0]
    tick["Views"]["Head"]["Side"][1] = 0
    tick["Players"]["Handle"][0] = 99
    tick["Match"]["GameTick"] = 101
    tick["Header"]["ActionTick"] = 13
    assert published.views["Head"]["Side"].tolist() == [1, 0]
    assert published.players["Handle"].tolist() == [7, 8]
    assert int(published.match["GameTick"]) == 100 and published.action_tick == 12 and int(published.header["ActionTick"]) == 12
    assert bridge.last_copy_ns > 0


def test_the_omniscient_blocks_are_not_copied_nor_read(region):
    bridge, _, _ = region
    published = bridge.read_tick_view()
    for name in ("entities", "weapons", "projectiles", "explosives", "lifts"):
        assert len(getattr(published, name)) == 0
    # The whole copy of read_tick still has them.
    assert len(bridge.read_tick().entities) == 3


def test_the_validation_passes_with_no_change_and_fails_with_a_changed_or_odd_sequence(region):
    bridge, _, _ = region
    tick = bridge.buffers[0]
    published = bridge.read_tick_view()
    assert bridge.is_unchanged(published)
    tick["Sequence"] = 3  # the writer started the next write
    assert not bridge.is_unchanged(published)
    tick["Sequence"] = 4
    tick["SequenceEnd"] = 4  # a whole new tick
    assert not bridge.is_unchanged(published)
    tick["Sequence"], tick["SequenceEnd"] = 2, 3  # SequenceEnd alone
    assert not bridge.is_unchanged(published)
    tick["SequenceEnd"] = 2
    assert bridge.is_unchanged(published)


def test_a_whole_copy_is_never_torn(region):
    bridge, _, _ = region
    published = bridge.read_tick()
    assert published.buffer_index is None and bridge.is_unchanged(published)
    bridge.buffers[0]["Sequence"] = 6
    assert bridge.is_unchanged(published)


def test_a_tick_being_written_is_not_read(region):
    bridge, _, _ = region
    bridge.buffers[0]["Sequence"] = 3
    assert bridge.read_tick_view() is None and bridge.tears > 0
    bridge.buffers[0]["Sequence"] = 2
    bridge.buffers[0]["SequenceEnd"] = 1  # the copies would not be one snapshot
    tears = bridge.tears
    assert bridge.read_tick_view() is None and bridge.tears > tears


def test_reread_gives_the_new_tick_and_wait_tick_goes_on_after_it(region):
    bridge, _, _ = region
    bridge.artemis["PublishedTicks"] = 1
    published = bridge.wait_tick(0.1, copy=False)
    assert published.buffer_index == 0 and published.skipped == 0 and bridge.last_published_ticks == 1
    tick = bridge.buffers[0]
    tick["Sequence"] = tick["SequenceEnd"] = 4
    tick["Header"]["ActionTick"] = 16
    bridge.artemis["PublishedTicks"] = 2
    assert not bridge.is_unchanged(published)
    again = bridge.reread_tick()
    assert again.action_tick == 16 and again.sequence == 4 and bridge.is_unchanged(again)
    assert bridge.last_published_ticks == 2
    assert bridge.wait_tick(0.01, copy=False) is None  # nothing newer
    # The default is still the whole copy.
    bridge.artemis["PublishedTicks"] = 3
    assert bridge.wait_tick(0.1).buffer_index is None
