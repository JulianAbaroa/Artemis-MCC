"""
tdump_writer.py
Test support of M2 (Ignore Rediseno/Tareas/M2.txt): writes .tdump files (format version 1, Ignore Referencia/Diseno/15 sec. 4) the
way the dump thread of Artemis does (Artemis/4_Bridge/Codec/Dump/System/Export.Dump.System.cpp: keyframe first and every
keyframe_interval records, otherwise the rows that are new or differ byte for byte plus the handles that are gone). The layout is the
real one of Data/Bridge/Bridge.xml (GenerateBridge.LayoutJson, the same text the C++ embeds), optionally changed to test layout
changes. It does not import tickdump.py: a test of the reader must not use the reader to write its input.
"""
import copy
import json
import os
import struct
import sys

import numpy as np


def Repo():
    """Folder of the project (the one with Tools/tickdump.py), found from this file."""
    folder = os.path.dirname(os.path.abspath(__file__))
    while not os.path.isfile(os.path.join(folder, "Tools", "tickdump.py")):
        parent = os.path.dirname(folder)
        if parent == folder:
            raise RuntimeError("Tools/tickdump.py not found above " + __file__)
        folder = parent
    return folder


sys.path.insert(0, os.path.join(Repo(), "Tools", "TagTranspiler"))
import GenerateBridge  # noqa: E402

FILE_HEADER = struct.Struct("<8sIIQIIII64s32s64s")
RECORD_HEADER = struct.Struct("<4sIIHHHHI")
FOOTER = struct.Struct("<4sIQQQQ")
HAS_MATCH, EVENTS_LOST, KEYFRAME = 1, 2, 4
DUMP_STRUCTS = ("BridgeVec3", "BridgeTickHeader", "BridgeMatch", "BridgePlayer", "BridgeEntity", "BridgeEvent")
SIZES = {"u8": 1, "i8": 1, "u16": 2, "i16": 2, "u32": 4, "f32": 4, "u64": 8}


def RealLayout():
    """(layout dict, LayoutHash) of Data/Bridge/Bridge.xml as GenerateBridge embeds it in the C++."""
    model = GenerateBridge.Read()
    structs = GenerateBridge.Layout(model)
    layout_hash = GenerateBridge.Hash(model, structs)
    return json.loads(GenerateBridge.LayoutJson(model, structs, layout_hash)), layout_hash


def ChangedLayout(layout, add=None, drop=None, retype=None):
    """A copy of layout with fields changed: add {struct: [(name, type)]} at the end, drop {struct: [name]}, retype {struct: {name:
    type}}. The offsets and the size of each changed struct are laid out again with natural alignment (as GenerateBridge does).
    Only the structs of a dump are kept (the ones that contain a table, like BridgeTick, would need the whole layout again)."""
    out = copy.deepcopy(layout)
    out["structs"] = [s for s in out["structs"] if s["name"] in DUMP_STRUCTS]
    sizes = {s["name"]: (s["size"], s["align"]) for s in out["structs"]}
    for s in out["structs"]:
        name = s["name"]
        if not any(name in (m or {}) for m in (add, drop, retype)):
            continue
        fields = [f for f in s["fields"] if f[0] not in (drop or {}).get(name, ())]
        for f in fields:
            f[1] = (retype or {}).get(name, {}).get(f[0], f[1])
        fields += [[n, t, 0, 1, ""] for n, t in (add or {}).get(name, ())]
        offset, align = 0, 1
        for f in fields:
            size, falign = (SIZES[f[1]], SIZES[f[1]]) if f[1] in SIZES else sizes[f[1]]
            offset = (offset + falign - 1) // falign * falign
            f[2] = offset
            offset += size * f[3]
            align = max(align, falign)
        s["fields"], s["size"], s["align"] = fields, (offset + align - 1) // align * align, align
        sizes[name] = (s["size"], s["align"])
    return out


def Dtypes(layout):
    out = {}
    for s in layout["structs"]:
        names, formats, offsets = [], [], []
        for name, type, offset, count, _ in s["fields"]:
            base = layout["primitives"].get(type) or out[type]
            names.append(name)
            formats.append((base, (count,)) if count != 1 else base)
            offsets.append(offset)
        out[s["name"]] = np.dtype({"names": names, "formats": formats, "offsets": offsets, "itemsize": s["size"]})
    return out


class Writer:
    """Builds a .tdump in memory, one Tick call per recorded tick. Rows are dicts {field: value}; missing fields are 0, a vector is a
    tuple, an array a list. PlayerCount and EntityCount of the header are always the row counts; EventCount and LastEventSequence
    default to the new events of the tick and the highest Sequence so far."""

    def __init__(self, layout=None, layout_hash=None, map_name="m10", source="manual", start_time="2026-10-23 14:00:00",
                 keyframe_interval=600):
        if layout is None:
            layout, real_hash = RealLayout()
            layout_hash = real_hash if layout_hash is None else layout_hash
        self.layout, self.layout_hash = layout, layout_hash or 0
        self.types = Dtypes(layout)
        self.map_name, self.source, self.start_time = map_name, source, start_time
        self.keyframe_interval, self.since_keyframe = keyframe_interval, keyframe_interval
        self.previous = {}
        self.chunks, self.recorded, self.dropped, self.event_losses, self.last_sequence = [], 0, 0, 0, 0

    def Rows(self, name, rows):
        out = np.zeros(len(rows), self.types[name])
        for i, row in enumerate(rows):
            for field, value in row.items():
                out[field][i] = value
        return out

    def Tick(self, game_tick, players=(), entities=(), events=(), header=None, match=None, dropped_before=0, has_match=True,
             events_lost=False):
        players, events = list(players), list(events)
        entity_rows = self.Rows("BridgeEntity", list(entities))
        entity_rows = entity_rows[np.argsort(entity_rows["Handle"], kind="stable")]
        self.last_sequence = max([self.last_sequence] + [int(e["Sequence"]) for e in events if "Sequence" in e])
        head = {"EventCount": len(events), "LastEventSequence": self.last_sequence}
        head.update(header or {})
        head.update({"PlayerCount": len(players), "EntityCount": len(entity_rows)})
        match_row = dict(match or {})
        if has_match:
            match_row.setdefault("GameTick", game_tick)

        flags = (HAS_MATCH if has_match else 0) | (EVENTS_LOST if events_lost else 0)
        if self.since_keyframe >= self.keyframe_interval:
            flags |= KEYFRAME
            self.since_keyframe = 0
            changed, removed = entity_rows, []
        else:
            current = {int(r["Handle"]): r.tobytes() for r in entity_rows}
            changed = entity_rows[[current[int(h)] != self.previous.get(int(h)) for h in entity_rows["Handle"]]] \
                if len(entity_rows) else entity_rows
            removed = sorted(h for h in self.previous if h not in current)
        self.since_keyframe += 1
        self.previous = {int(r["Handle"]): r.tobytes() for r in entity_rows}

        self.chunks.append(RECORD_HEADER.pack(b"TICK", game_tick if has_match else 0, dropped_before, len(events), flags,
                                              len(changed), len(removed), 0))
        for name, rows in (("BridgeTickHeader", [head]), ("BridgeMatch", [match_row]), ("BridgePlayer", players)):
            self.chunks.append(self.Rows(name, rows).tobytes())
        self.chunks.append(changed.tobytes())
        self.chunks.append(np.array(removed, "<u4").tobytes())
        self.chunks.append(self.Rows("BridgeEvent", events).tobytes())
        self.recorded += 1
        self.dropped += dropped_before
        self.event_losses += 1 if events_lost else 0
        return self

    def Bytes(self, footer=True, dropped_ticks=None, event_losses=None, queue_waits=0, cut=0):
        """The whole file. footer=False: a recording that was cut (crash). cut=N drops the last N bytes."""
        text = json.dumps(self.layout, separators=(",", ":")).encode("ascii")
        head = FILE_HEADER.pack(b"ATDUMP\0\0", 1, FILE_HEADER.size, self.layout_hash, self.layout.get("version", 1), len(text),
                                RECORD_HEADER.size, 0, self.map_name.encode(), self.start_time.encode(), self.source.encode())
        data = head + text + b"".join(self.chunks)
        if footer:
            data += FOOTER.pack(b"TEND", 0, self.recorded, self.dropped if dropped_ticks is None else dropped_ticks, queue_waits,
                                self.event_losses if event_losses is None else event_losses)
        return data[:len(data) - cut]

    def Save(self, path, **kwargs):
        with open(path, "wb") as handle:
            handle.write(self.Bytes(**kwargs))
        return str(path)
