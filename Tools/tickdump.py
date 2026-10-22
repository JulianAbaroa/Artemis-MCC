"""
tickdump.py
Reads the .tdump files of the TickDump (D151, Ignore Diseno/15): the BridgeTick of every tick, with the layout of Bridge.xml embedded
in the file, so an old dump stays readable after Bridge.xml changes. Output is short on purpose (it is read by Claude).

    python Tools/tickdump.py show [FILE]                  Summary: source, ticks, GameTick range and gaps, drops, events by kind.
    python Tools/tickdump.py show [FILE] --tick T         One record: header, match, players and the entity count.
    python Tools/tickdump.py show [FILE] --handle H [--field F]   One entity or player over the dump (first, last and changes).

FILE defaults to the newest file of %LOCALAPPDATA%\\Artemis\\TickDumps (run it on Windows: python Tools/game.py py Tools/tickdump.py
show). diff and approve come with F1b (Diseno/15 sec. 10).
"""
import argparse
import glob
import json
import os
import struct
import sys
from collections import Counter

import numpy as np

FILE_HEADER = struct.Struct("<8sIIQIIII64s32s64s")
RECORD_HEADER = np.dtype([("Marker", "S4"), ("GameTick", "<u4"), ("DroppedBefore", "<u4"), ("NewEventCount", "<u2"), ("Flags", "<u2"),
                          ("ChangedEntityCount", "<u2"), ("RemovedEntityCount", "<u2"), ("Reserved", "<u4")])
FOOTER = np.dtype([("Marker", "S4"), ("Reserved", "<u4"), ("RecordedTicks", "<u8"), ("DroppedTicks", "<u8"), ("QueueWaits", "<u8"),
                   ("EventLosses", "<u8")])
HAS_MATCH, EVENTS_LOST, KEYFRAME = 1, 2, 4


def Text(raw):
    return raw.split(b"\0", 1)[0].decode("utf-8", "replace")


def Dtypes(layout):
    """numpy dtypes of every struct of the embedded layout (Bridge.xml at the time of the recording). The structs come in
    dependency order and the primitives carry their numpy format, so the file alone is enough."""
    PRIMITIVES = layout["primitives"]
    out = {}
    for s in layout["structs"]:
        names, formats, offsets = [], [], []
        for name, type, offset, count, _ in s["fields"]:
            base = PRIMITIVES.get(type) or out[type]
            names.append(name)
            formats.append((base, (count,)) if count != 1 else base)
            offsets.append(offset)
        out[s["name"]] = np.dtype({"names": names, "formats": formats, "offsets": offsets, "itemsize": s["size"]})
    return out


class Record:
    # entities: the delta of the file (new or changed rows), removed: handles gone since the previous record. Dump.states() rebuilds
    # the full table of every tick.
    __slots__ = ("game_tick", "dropped", "flags", "header", "match", "players", "entities", "removed", "events")


class Dump:
    """A .tdump read whole: header fields, the layout and one Record per tick."""

    def __init__(self, path):
        self.path = path
        data = open(path, "rb").read()
        (magic, self.version, header_size, self.layout_hash, self.layout_version, json_size, record_size, _, map_name, start,
         source) = FILE_HEADER.unpack_from(data, 0)
        if magic != b"ATDUMP\0\0":
            sys.exit(f"ERROR: {path} is not a .tdump file.")
        if self.version != 1:
            sys.exit(f"ERROR: {path} has format version {self.version}, this reader knows 1.")
        self.map_name, self.start_time, self.source = Text(map_name), Text(start), Text(source)
        self.layout = json.loads(data[header_size:header_size + json_size])
        types = Dtypes(self.layout)
        th, tm, tp, te, tv = (types[n] for n in ("BridgeTickHeader", "BridgeMatch", "BridgePlayer", "BridgeEntity", "BridgeEvent"))
        self.records, self.footer, self.is_cut = [], None, False
        offset = header_size + json_size
        while offset < len(data):
            marker = data[offset:offset + 4]
            if marker == b"TEND":
                if offset + FOOTER.itemsize <= len(data):
                    self.footer = np.frombuffer(data, FOOTER, 1, offset)[0]
                else:
                    self.is_cut = True
                break
            if marker != b"TICK" or offset + record_size + th.itemsize + tm.itemsize > len(data):
                self.is_cut = True
                break
            r = Record()
            rh = np.frombuffer(data, RECORD_HEADER, 1, offset)[0]
            offset += record_size
            r.game_tick, r.dropped, r.flags = int(rh["GameTick"]), int(rh["DroppedBefore"]), int(rh["Flags"])
            r.header = np.frombuffer(data, th, 1, offset)[0]
            offset += th.itemsize
            r.match = np.frombuffer(data, tm, 1, offset)[0]
            offset += tm.itemsize
            counts = ((tp, int(r.header["PlayerCount"])), (te, int(rh["ChangedEntityCount"])),
                      (np.dtype("<u4"), int(rh["RemovedEntityCount"])), (tv, int(rh["NewEventCount"])))
            if offset + sum(t.itemsize * n for t, n in counts) > len(data):
                self.is_cut = True
                break
            rows = []
            for t, n in counts:
                rows.append(np.frombuffer(data, t, n, offset))
                offset += t.itemsize * n
            r.players, r.entities, r.removed, r.events = rows
            self.records.append(r)
        else:
            self.is_cut = self.footer is None
        self.size = len(data)

    def states(self):
        """(record, entities of that tick, sorted by handle) for every record, rebuilt from the keyframes and the deltas."""
        table = None
        for r in self.records:
            if r.flags & KEYFRAME or table is None:
                table = r.entities.copy()
            else:
                if len(r.removed):
                    table = table[~np.isin(table["Handle"], r.removed)]
                if len(r.entities):
                    table = table[~np.isin(table["Handle"], r.entities["Handle"])]
                    table = np.concatenate([table, r.entities])
                    table = table[np.argsort(table["Handle"], kind="stable")]
            yield r, table


def Newest():
    folder = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Artemis", "TickDumps")
    files = sorted(glob.glob(os.path.join(folder, "*.tdump")), key=os.path.getmtime)
    if not files:
        sys.exit(f"ERROR: no .tdump in {folder}.")
    return files[-1]


def EventNames():
    """Event kind -> name from the generated Agent/artemis/semantic.py, if it can be imported."""
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Agent"))
    try:
        from artemis.semantic import Event
        return lambda k: Event(k).name if k in Event._value2member_map_ else str(k)
    except Exception:
        return str


def Summary(d):
    r = d.records
    ticks = [x.game_tick for x in r if x.flags & HAS_MATCH]
    gaps = [(a, b) for a, b in zip(ticks, ticks[1:]) if b != a + 1]
    name = EventNames()
    kinds = Counter(name(int(k)) for x in r for k in x.events["Kind"]) if r else Counter()
    entities = [int(x.header["EntityCount"]) for x in r]
    print(f"{os.path.basename(d.path)} | map {d.map_name} | source {d.source} | start {d.start_time} | layout 0x{d.layout_hash:016X}"
          f" | {d.size / 1e6:.1f} MB, {d.size / max(len(r), 1) / 1e3:.1f} KB per tick")
    print(f"records {len(r)} (with match {len(ticks)}), GameTick {ticks[0] if ticks else '-'}-{ticks[-1] if ticks else '-'}, "
          f"gaps {len(gaps)}{' first ' + str(gaps[0]) if gaps else ''}, repeated {len(ticks) - len(set(ticks))}")
    print(f"dropped {sum(x.dropped for x in r)}, records with lost events {sum(1 for x in r if x.flags & EVENTS_LOST)}, "
          f"entities avg {np.mean(entities) if entities else 0:.0f} max {max(entities, default=0)}, "
          f"players max {max((len(x.players) for x in r), default=0)}")
    print(f"events {sum(kinds.values())}: {dict(kinds.most_common(12))}")
    bad = [x.game_tick for x, table in d.states() if len(table) != int(x.header["EntityCount"])]
    if bad:
        print(f"ERROR: rebuilt entity count differs from EntityCount in {len(bad)} records, first GameTick {bad[0]}")
    if d.footer is not None:
        f = d.footer
        print(f"footer: recorded {int(f['RecordedTicks'])}, dropped {int(f['DroppedTicks'])}, queue waits {int(f['QueueWaits'])}, "
              f"event losses {int(f['EventLosses'])}")
    else:
        print(f"footer: MISSING (the recording was cut{', the last record is incomplete' if d.is_cut else ''})")


def Row(row, fields=None):
    names = fields or row.dtype.names
    parts = []
    for n in names:
        v = row[n]
        if v.dtype.names:
            v = "(" + ",".join(f"{float(v[c]):.3f}" for c in v.dtype.names) + ")"
        elif v.dtype.kind == "f":
            v = f"{float(v):.4f}"
        elif n in ("Handle", "BoundTo", "ParentHandle", "BipedHandle", "Objective") or n.endswith("Handle"):
            v = f"{int(v):08X}" if v.ndim == 0 else v
        parts.append(f"{n}={v}")
    return " ".join(parts)


def ShowTick(d, tick):
    found = next(((r, t) for r, t in d.states() if r.flags & HAS_MATCH and r.game_tick == tick), None)
    if found is None:
        sys.exit(f"ERROR: no record with GameTick {tick}.")
    r, table = found
    print(f"GameTick {tick} dropped {r.dropped} flags {r.flags} | {Row(r.header)}")
    print(f"match: {Row(r.match)}")
    for p in r.players:
        print(f"player: {Row(p)}")
    print(f"entities {len(table)} ({len(r.entities)} written in this record), new events {len(r.events)}")
    for e in r.events:
        print(f"event: {Row(e)}")


def ShowHandle(d, handle, field):
    seen, last = 0, None
    for r, entities in d.states():
        for table, label in ((r.players, "player"), (entities, "entity")):
            rows = table[table["Handle"] == handle] if len(table) else table
            if not len(rows):
                continue
            row = rows[0]
            text = Row(row, [field] if field else None)
            if text != last:
                if seen < 40:
                    print(f"{r.game_tick} {label}: {text}")
                seen += 1
                last = text
    print(f"{seen} distinct values" if seen else f"handle {handle:08X} not found")


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["show"])
    parser.add_argument("file", nargs="?")
    parser.add_argument("--tick", type=int)
    parser.add_argument("--handle", type=lambda s: int(s, 16))
    parser.add_argument("--field")
    args = parser.parse_args()
    d = Dump(args.file or Newest())
    if args.tick is not None:
        ShowTick(d, args.tick)
    elif args.handle is not None:
        ShowHandle(d, args.handle, args.field)
    else:
        Summary(d)
    return 0


if __name__ == "__main__":
    sys.exit(Main())
