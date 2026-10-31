"""
tickdump.py
Reads the .tdump files of the TickDump (D151, Ignore Diseno/15): the BridgeTick of every tick, with the layout of Bridge.xml embedded
in the file, so an old dump stays readable after Bridge.xml changes. Output is short on purpose (it is read by Claude).

    python Tools/tickdump.py show [FILE]                  Summary: source, ticks, GameTick range and gaps, drops, events by kind.
    python Tools/tickdump.py show [FILE] --tick T         One record: header, match, players and the entity count.
    python Tools/tickdump.py show [FILE] --handle H [--field F]   One entity or player over the dump (first, last and changes).
    python Tools/tickdump.py diff GOLDEN RUN [--overlap] [--max N]
        Exact comparison by bits, field by field, tick by tick (D170, D171). Exit 0 equal, 1 differs, 2 error.
    python Tools/tickdump.py approve RUN GOLDEN_DIR --reason "..."
        Saves RUN as GOLDEN_DIR/golden.tdump (the previous one becomes golden_prev.tdump) and adds a line to manifest.txt (D172).
        The .sdump of the same session (RUN with the extension .sdump) is saved as golden.sdump and named in the line. A run
        without .sdump deletes an old golden.sdump, so the two files never belong to different recordings (M5c).

FILE defaults to the newest file of %LOCALAPPDATA%\\Artemis\\TickDumps (run it on Windows: python Tools/game.py py Tools/tickdump.py
show). A folder stands for <folder>/golden.tdump; a path that does not exist is looked up in that TickDumps folder (the file name
alone is enough). Errors print "ERROR: ..." and exit 2.

Diff rules: rows are aligned by GameTick (records with Match only), Players and Entities by Handle, events by Sequence. Fields are
matched by name across the two embedded layouts: an added, removed or retyped field is a layout change (+, -, ~) and counts as a
difference, the common fields keep being compared. Floats compare by bits (NaN equals the same NaN, 0 differs from -0). IGNORED are
the fields of the local view of the theater and of the bridge clock. Ticks that are only in one file count as differences unless
--overlap (a recording armed by hand starts late); events are compared when their GameTick falls inside the compared range.
A GameTick that goes back (a seek of the theater), a repeated key in a record or a repeated Sequence is an error, not a difference.
D174: a GameTick repeated in sequence is valid (the theater runs records on GameTick 0 while it loads and repeats the last one); the
records align by (GameTick, occurrence) and the output writes "0#5" for the sixth record of tick 0.
"""
import argparse
import datetime
import glob
import json
import os
import shutil
import struct
import subprocess
import sys
from collections import Counter
from dataclasses import dataclass
from typing import Optional

import numpy as np

FILE_HEADER = struct.Struct("<8sIIQIIII64s32s64s")
RECORD_HEADER = np.dtype([("Marker", "S4"), ("GameTick", "<u4"), ("DroppedBefore", "<u4"), ("NewEventCount", "<u2"), ("Flags", "<u2"),
                          ("ChangedEntityCount", "<u2"), ("RemovedEntityCount", "<u2"), ("Reserved", "<u4")])
FOOTER = np.dtype([("Marker", "S4"), ("Reserved", "<u4"), ("RecordedTicks", "<u8"), ("DroppedTicks", "<u8"), ("QueueWaits", "<u8"),
                   ("EventLosses", "<u8")])
HAS_MATCH, EVENTS_LOST, KEYFRAME = 1, 2, 4
# Tables the layout of a file may not have, in the order of the blocks at the end of a record: Record slot, struct, count in the header.
# Version 2 added the first two, version 3 the last two. A file whose layout lacks a block has none of it.
EXTRA_BLOCKS = (("objectives", "BridgeObjective", "ObjectiveCount"), ("stands", "BridgeStand", "StandCount"),
                ("weapons", "BridgeWeapon", "WeaponCount"), ("projectiles", "BridgeProjectile", "ProjectileCount"))
# Rows of a table the layout of a file does not have (a dump from before the flags): empty, with the key every table has.
EMPTY_ROWS = np.dtype([("Handle", "<u4")])
# Table name in the output -> struct of the layout (Bridge.xml).
TABLES = {"Header": "BridgeTickHeader", "Match": "BridgeMatch", "Players": "BridgePlayer", "Entities": "BridgeEntity",
          "Events": "BridgeEvent", "Objectives": "BridgeObjective", "Stands": "BridgeStand", "Weapons": "BridgeWeapon",
          "Projectiles": "BridgeProjectile"}
# Tables of the Diff that are compared tick by tick (the events are compared apart, by Sequence).
TICK_TABLES = ("Header", "Match", "Players", "Entities", "Objectives", "Stands", "Weapons", "Projectiles")
# Fields that are not compared: the local view of the theater and the clock of the bridge (D170). Listed in every diff.
IGNORED = {"Header": ("MatchId", "ActionTick", "Generation", "PublishQpc"),
           "Players": ("LocalIndex", "Connection", "CameraPosition", "CameraForward")}


class TickDumpError(Exception):
    """Any error of reading, of range or of approve. Main prints it as "ERROR: ..." and exits with 2 (1 means "differs")."""


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
    __slots__ = ("game_tick", "dropped", "flags", "header", "match", "players", "entities", "removed", "events", "objectives", "stands", "weapons",
                 "projectiles")


class Dump:
    """A .tdump read whole: header fields, the layout and one Record per tick."""

    def __init__(self, path):
        self.path = path
        try:
            with open(path, "rb") as handle:
                data = handle.read()
        except OSError as error:
            raise TickDumpError(f"cannot read {path}: {error.strerror or error}")
        if len(data) < FILE_HEADER.size:
            raise TickDumpError(f"{path} is not a .tdump file.")
        (magic, self.version, header_size, self.layout_hash, self.layout_version, json_size, record_size, _, map_name, start,
         source) = FILE_HEADER.unpack_from(data, 0)
        if magic != b"ATDUMP\0\0":
            raise TickDumpError(f"{path} is not a .tdump file.")
        if self.version not in (1, 2, 3):
            raise TickDumpError(f"{path} has format version {self.version}, this reader knows 1, 2 and 3.")
        self.map_name, self.start_time, self.source = Text(map_name), Text(start), Text(source)
        try:
            self.layout = json.loads(data[header_size:header_size + json_size])
            self.types = Dtypes(self.layout)
            th, tm, tp, te, tv = (self.types[n] for n in ("BridgeTickHeader", "BridgeMatch", "BridgePlayer", "BridgeEntity",
                                                          "BridgeEvent"))
            # Version 2 puts the flags and the stands, version 3 the weapons and the projectiles (counts in the header), after the
            # events. A layout without a block has no such block.
            blocks = [(slot, self.types[struct], count) for slot, struct, count in EXTRA_BLOCKS
                      if struct in self.types and count in th.names]
        except (ValueError, KeyError, TypeError, IndexError) as error:
            raise TickDumpError(f"{path} has an unreadable layout ({type(error).__name__}: {error}).")
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
            counts += tuple((t, int(r.header[count])) for _, t, count in blocks)
            if offset + sum(t.itemsize * n for t, n in counts) > len(data):
                self.is_cut = True
                break
            rows = []
            for t, n in counts:
                rows.append(np.frombuffer(data, t, n, offset))
                offset += t.itemsize * n
            r.players, r.entities, r.removed, r.events = rows[:4]
            present = dict(zip((slot for slot, _, _ in blocks), rows[4:]))
            for slot, _, _ in EXTRA_BLOCKS:
                setattr(r, slot, present.get(slot, np.zeros(0, EMPTY_ROWS)))
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


def DumpsFolder():
    return os.path.join(os.environ.get("LOCALAPPDATA", ""), "Artemis", "TickDumps")


def Newest():
    folder = DumpsFolder()
    files = sorted(glob.glob(os.path.join(folder, "*.tdump")), key=os.path.getmtime)
    if not files:
        raise TickDumpError(f"no .tdump in {folder}.")
    return files[-1]


def ResolvePath(arg):
    """A folder is <folder>/golden.tdump; a path that does not exist is looked up by its file name in the TickDumps folder."""
    if os.path.isdir(arg):
        golden = os.path.join(arg, "golden.tdump")
        if not os.path.isfile(golden):
            raise TickDumpError(f"{arg} has no golden.tdump.")
        return golden
    if os.path.isfile(arg):
        return arg
    candidate = os.path.join(DumpsFolder(), os.path.basename(arg))
    if os.path.isfile(candidate):
        return candidate
    raise TickDumpError(f"{arg} not found (also looked in {DumpsFolder()}).")


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
        if v.dtype.names and v.ndim:
            v = "[" + ";".join("(" + ",".join(f"{int(x[c])}" for c in v.dtype.names) + ")" for x in v) + "]"
        elif v.dtype.names:
            v = "(" + ",".join(f"{float(v[c]):.3f}" for c in v.dtype.names) + ")"
        elif v.dtype.kind == "f":
            v = f"{float(v):.4f}"
        elif n in ("Handle", "BoundTo", "ParentHandle", "BipedHandle", "Objective", "CarrierPlayer", "CarrierBiped", "HomeStand",
                 "TrackedObject", "OwnerPlayer", "OwnerBiped", "ShooterRoot", "StuckTo") \
                or n.endswith("Handle"):
            v = f"{int(v):08X}" if v.ndim == 0 else v
        parts.append(f"{n}={v}")
    return " ".join(parts)


def ShowTick(d, tick):
    found = next(((r, t) for r, t in d.states() if r.flags & HAS_MATCH and r.game_tick == tick), None)
    if found is None:
        raise TickDumpError(f"no record with GameTick {tick}.")
    r, table = found
    print(f"GameTick {tick} dropped {r.dropped} flags {r.flags} | {Row(r.header)}")
    print(f"match: {Row(r.match)}")
    for p in r.players:
        print(f"player: {Row(p)}")
    for o in r.objectives:
        print(f"objective: {Row(o)}")
    for s in r.stands:
        print(f"stand: {Row(s)}")
    for w in r.weapons:
        print(f"weapon: {Row(w)}")
    for p in r.projectiles:
        print(f"projectile: {Row(p)}")
    print(f"entities {len(table)} ({len(r.entities)} written in this record), new events {len(r.events)}")
    for e in r.events:
        print(f"event: {Row(e)}")


def ShowHandle(d, handle, field):
    seen, last = 0, None
    for r, entities in d.states():
        for table, label in ((r.players, "player"), (entities, "entity"), (r.objectives, "objective"), (r.stands, "stand"),
                             (r.weapons, "weapon"), (r.projectiles, "projectile")):
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


# --- diff (D170, D171) ---------------------------------------------------------------------------------------------------------

@dataclass
class FieldDiff:
    ticks: int
    keys: int
    first_tick: int
    first_key: Optional[int]
    golden: str
    run: str
    first_occurrence: int = 0


@dataclass
class Existence:
    only_golden: int
    only_run: int
    first_tick: int
    first_key: int
    first_side: str
    first_occurrence: int = 0


@dataclass
class DiffResult:
    golden_path: str
    run_path: str
    golden_layout: int
    run_layout: int
    overlap: bool
    schema: list
    tick_range: Optional[tuple]
    ticks_compared: int
    only_golden_ticks: list
    only_run_ticks: list
    fields: dict
    existence: dict
    ignored: list

    @property
    def is_equal(self):
        return (self.ticks_compared > 0 and not self.schema and not self.fields and not self.existence
                and (self.overlap or not (self.only_golden_ticks or self.only_run_ticks)))


def FormatValue(v):
    """Text of one value: f32 shortest round trip ("0.5", "1", "-0", "nan"), integers in decimal, "(x,y,z)", "[a,b,...]"."""
    if isinstance(v, np.ndarray):
        return "[" + ",".join(FormatValue(x) for x in v) + "]"
    if v.dtype.names:
        return "(" + ",".join(FormatValue(v[n]) for n in v.dtype.names) + ")"
    if v.dtype.kind == "f":
        return np.format_float_positional(v, unique=True, trim="-")
    return str(int(v))


def Bits(column):
    """The bytes of a column as (rows, n) uint8, field by field so that the padding of a struct is not compared."""
    if column.dtype.names:
        return np.concatenate([Bits(column[n]) for n in column.dtype.names], axis=1)
    return np.ascontiguousarray(column).view(np.uint8).reshape(len(column), -1)


def DifferentBits(a, b):
    return (Bits(a) != Bits(b)).any(axis=1)


def RawRows(rows):
    rows = np.ascontiguousarray(rows)
    return rows.view(np.uint8).reshape(len(rows), -1)


def OneRow(row):
    return np.array([row], dtype=row.dtype)


def LayoutFields(dump, struct_name):
    for s in dump.layout["structs"]:
        if s["name"] == struct_name:
            return [(f[0], (f[1], f[3])) for f in s["fields"]]
    return []


def TypeText(t):
    return t[0] if t[1] == 1 else f"{t[0]}[{t[1]}]"


def Schema(golden, run):
    """(layout changes as text, {table: compared field names in the order of the golden layout})."""
    schema, common = [], {}
    for table, struct_name in TABLES.items():
        g, r = LayoutFields(golden, struct_name), LayoutFields(run, struct_name)
        if not g or not r:
            # A table one layout does not have (the flags in a dump from before them): one line, and nothing to compare.
            if g or r:
                schema.append(f"{'-' if g else '+'} {table} (table of {len(g or r)} fields)")
            common[table] = []
            continue
        gd, rd = dict(g), dict(r)
        names = []
        for name, t in g:
            if name not in rd:
                schema.append(f"- {table}.{name}")
            elif rd[name] != t:
                schema.append(f"~ {table}.{name} {TypeText(t)} -> {TypeText(rd[name])}")
            elif name not in IGNORED.get(table, ()):
                names.append(name)
        schema += [f"+ {table}.{name}" for name, _ in r if name not in gd]
        common[table] = names
    return schema, common


def MatchTicks(dump):
    """GameTick of the records with Match, never going back (D174: equal ones in sequence are valid; a smaller one is a seek)."""
    ticks = [r.game_tick for r in dump.records if r.flags & HAS_MATCH]
    for a, b in zip(ticks, ticks[1:]):
        if b < a:
            raise TickDumpError(f"{os.path.basename(dump.path)}: GameTick {b} comes after {a} (going back: a seek of the theater, "
                                f"or two matches in one file).")
    return ticks


def Occurrences(ticks):
    """(GameTick, occurrence) of each record: occurrence = how many times that GameTick appeared before in the file."""
    seen, keys = {}, []
    for t in ticks:
        keys.append((t, seen.get(t, 0)))
        seen[t] = seen.get(t, 0) + 1
    return keys


def MatchStates(dump, keys):
    """(key, record, entities) for every record with Match; keys come from Occurrences(MatchTicks(dump))."""
    keys = iter(keys)
    for r, table in dump.states():
        if r.flags & HAS_MATCH:
            yield next(keys), r, table


def SortedByHandle(rows, where):
    handles = rows["Handle"]
    if len(handles) > 1:
        if (handles[1:] < handles[:-1]).any():
            rows = rows[np.argsort(handles, kind="stable")]
            handles = rows["Handle"]
        if (handles[1:] == handles[:-1]).any():
            raise TickDumpError(f"{where}: a Handle is repeated.")
    return rows


class FieldAcc:
    """What a diff keeps of one field over the ticks: ticks and keys affected and the first difference."""

    def __init__(self):
        self.ticks, self.keys, self.first = 0, set(), None

    def Add(self, tick, keys, mask, golden, run):
        tick, occurrence = tick
        index = np.nonzero(mask)[0]
        self.ticks += 1
        if keys is not None:
            self.keys.update(keys[index].tolist())
        if self.first is None:
            i = index[0]
            self.first = (tick, int(keys[i]) if keys is not None else None, FormatValue(golden[i]), FormatValue(run[i]), occurrence)

    def Result(self):
        tick, key, golden, run, occurrence = self.first
        return FieldDiff(self.ticks, len(self.keys), tick, key, golden, run, occurrence)


class TableDiff:
    """The comparison of one per-tick table (Header, Match, Players, Entities) over the common ticks."""

    def __init__(self, name, golden, run, fields):
        self.name, self.fields = name, fields
        self.accs = {f: FieldAcc() for f in fields}
        self.only = {"golden": set(), "run": set()}
        self.first = None
        self.where = (os.path.basename(golden.path), os.path.basename(run.path))

    def Existence(self, tick, only_golden, only_run):
        self.only["golden"].update(only_golden.tolist())
        self.only["run"].update(only_run.tolist())
        if self.first is None and (len(only_golden) or len(only_run)):
            candidates = [(int(only_golden.min()), "golden")] if len(only_golden) else []
            candidates += [(int(only_run.min()), "run")] if len(only_run) else []
            key, side = min(candidates)
            self.first = (tick[0], key, side, tick[1])

    def Compare(self, tick, golden, run, keyed):
        keys = None
        if keyed:
            golden = SortedByHandle(golden, f"{self.where[0]} tick {tick[0]}#{tick[1]} {self.name}")
            run = SortedByHandle(run, f"{self.where[1]} tick {tick[0]}#{tick[1]} {self.name}")
            hg, hr = golden["Handle"], run["Handle"]
            if len(hg) == len(hr) and np.array_equal(hg, hr):
                keys = hg
            else:
                keys, ig, ir = np.intersect1d(hg, hr, assume_unique=True, return_indices=True)
                self.Existence(tick, np.setdiff1d(hg, keys, assume_unique=True), np.setdiff1d(hr, keys, assume_unique=True))
                golden, run = golden[ig], run[ir]
        if not len(golden):
            return
        # With the same dtype in both arrays (numpy packs the dtype of a table rebuilt from deltas, so compare the arrays' own) a row
        # that is byte for byte equal, ignored fields zeroed, has nothing to look at.
        if golden.dtype == run.dtype:
            x, y = golden, run
            zero = [n for n in IGNORED.get(self.name, ()) if n in golden.dtype.names]
            if zero:
                x, y = x.copy(), y.copy()
                for n in zero:
                    x[n] = 0
                    y[n] = 0
            bad = (RawRows(x) != RawRows(y)).any(axis=1)
            if not bad.any():
                return
            index = np.nonzero(bad)[0]
            golden, run = golden[index], run[index]
            keys = keys[index] if keys is not None else None
        for name in self.fields:
            mask = DifferentBits(golden[name], run[name])
            if mask.any():
                self.accs[name].Add(tick, keys, mask, golden[name], run[name])


def AllEvents(dump):
    parts = [r.events for r in dump.records if len(r.events)]
    events = np.concatenate(parts) if parts else np.zeros(0, dump.types["BridgeEvent"])
    events = events[np.argsort(events["Sequence"], kind="stable")]
    sequences = events["Sequence"]
    if len(sequences) > 1 and (sequences[1:] == sequences[:-1]).any():
        raise TickDumpError(f"{os.path.basename(dump.path)}: an event Sequence is repeated.")
    return events


def DiffEvents(golden, run, lo, hi, fields):
    """(fields {name: FieldDiff in the order of fields}, Existence or None) of the events whose GameTick is in lo..hi."""
    eg, er = AllEvents(golden), AllEvents(run)
    sg, sr = eg["Sequence"], er["Sequence"]
    common, ig, ir = np.intersect1d(sg, sr, assume_unique=True, return_indices=True)
    in_range = lambda t: (t >= lo) & (t <= hi)
    only_g = np.setdiff1d(np.arange(len(eg)), ig)
    only_r = np.setdiff1d(np.arange(len(er)), ir)
    only_g = only_g[in_range(eg["GameTick"][only_g])]
    only_r = only_r[in_range(er["GameTick"][only_r])]
    cg, cr = eg[ig], er[ir]
    keep = np.nonzero(in_range(cg["GameTick"]) | in_range(cr["GameTick"]))[0]
    cg, cr, common = cg[keep], cr[keep], common[keep]
    out = {}
    if len(common):
        for name in fields:
            mask = DifferentBits(cg[name], cr[name])
            if not mask.any():
                continue
            index = np.nonzero(mask)[0]
            first = index[np.lexsort((common[index], cg["GameTick"][index]))[0]]
            out[name] = FieldDiff(len(np.unique(cg["GameTick"][index])), len(index), int(cg["GameTick"][first]), int(common[first]),
                                  FormatValue(cg[name][first]), FormatValue(cr[name][first]))
    existence = None
    if len(only_g) or len(only_r):
        candidates = [(int(eg["GameTick"][i]), int(sg[i]), "golden") for i in only_g]
        candidates += [(int(er["GameTick"][i]), int(sr[i]), "run") for i in only_r]
        tick, key, side = min(candidates)
        existence = Existence(len(only_g), len(only_r), tick, key, side)
    return out, existence


def Diff(golden, run, overlap=False):
    schema, common = Schema(golden, run)
    golden_keys, run_keys = Occurrences(MatchTicks(golden)), Occurrences(MatchTicks(run))
    both = set(golden_keys) & set(run_keys)
    tick_range = (min(k[0] for k in both), max(k[0] for k in both)) if both else None
    tables = {name: TableDiff(name, golden, run, common[name]) for name in TICK_TABLES}
    if both:
        last = max(both)
        gs, rs = MatchStates(golden, golden_keys), MatchStates(run, run_keys)
        g, r = next(gs, None), next(rs, None)
        while g is not None and r is not None and min(g[0], r[0]) <= last:
            if g[0] < r[0]:
                g = next(gs, None)
            elif g[0] > r[0]:
                r = next(rs, None)
            else:
                (key, rg, tg), (_, rr, tr) = g, r
                tables["Header"].Compare(key, OneRow(rg.header), OneRow(rr.header), False)
                tables["Match"].Compare(key, OneRow(rg.match), OneRow(rr.match), False)
                tables["Players"].Compare(key, rg.players, rr.players, True)
                tables["Entities"].Compare(key, tg, tr, True)
                for name, a, b in (("Objectives", rg.objectives, rr.objectives), ("Stands", rg.stands, rr.stands),
                                   ("Weapons", rg.weapons, rr.weapons), ("Projectiles", rg.projectiles, rr.projectiles)):
                    if tables[name].fields:
                        tables[name].Compare(key, a, b, True)
                g, r = next(gs, None), next(rs, None)
    event_fields, event_existence = {}, None
    if both:
        event_fields, event_existence = DiffEvents(golden, run, tick_range[0], tick_range[1], common["Events"])
    fields, existence = {}, {}
    for name in TABLES:
        if name == "Events":
            fields.update({f"Events.{f}": event_fields[f] for f in common["Events"] if f in event_fields})
            if event_existence:
                existence["Events"] = event_existence
            continue
        table = tables[name]
        fields.update({f"{name}.{f}": table.accs[f].Result() for f in table.fields if table.accs[f].first is not None})
        if table.first is not None:
            tick, key, side, occurrence = table.first
            existence[name] = Existence(len(table.only["golden"]), len(table.only["run"]), tick, key, side, occurrence)
    return DiffResult(golden.path, run.path, golden.layout_hash, run.layout_hash, overlap, schema, tick_range, len(both),
                      [k[0] for k in golden_keys if k not in both], [k[0] for k in run_keys if k not in both], fields, existence,
                      [f"{t}.{f}" for t, names in IGNORED.items() for f in names])


def Ranges(ticks):
    """"0-2, 10-12" (at most 4 ranges, then " ...")."""
    groups = []
    for t in ticks:
        if groups and t == groups[-1][1] + 1:
            groups[-1][1] = t
        else:
            groups.append([t, t])
    text = ", ".join(str(a) if a == b else f"{a}-{b}" for a, b in groups[:4])
    return text + (" ..." if len(groups) > 4 else "")


def FormatTick(tick, occurrence):
    return f"{tick}#{occurrence}" if occurrence else str(tick)


def FormatKey(table, key):
    return "" if key is None else (str(key) if table == "Events" else f"{key:08X}")


def FormatDiff(result, max_fields=20):
    """The lines of the report (Diseno/15 sec. 6): short, one per finding, the last one starts with EQUAL or DIFFERS."""
    name = os.path.basename
    lines = [f"golden {name(result.golden_path)} layout 0x{result.golden_layout:016X} | run {name(result.run_path)} layout "
             f"0x{result.run_layout:016X}"]
    span = f"{result.tick_range[0]}-{result.tick_range[1]}" if result.tick_range else "none"
    line = f"ticks compared {result.ticks_compared} ({span})"
    for label, ticks in (("golden", result.only_golden_ticks), ("run", result.only_run_ticks)):
        line += f" | {label} only {len(ticks)}" + (f" ({Ranges(ticks)})" if ticks else "")
    lines.append(line + (" | overlap" if result.overlap else ""))
    lines.append("ignored: " + ", ".join(result.ignored))
    if result.schema:
        lines.append("schema: " + "; ".join(result.schema))
    for key, f in list(result.fields.items())[:max_fields]:
        where = f"first tick {FormatTick(f.first_tick, f.first_occurrence)}" + (f" key {FormatKey(key.split('.')[0], f.first_key)}" if f.first_key is not None else "")
        lines.append(f"{key}: {f.ticks} ticks, {f.keys} keys, {where}: {f.golden} -> {f.run}")
    if len(result.fields) > max_fields:
        lines.append(f"... {len(result.fields) - max_fields} more fields")
    for table, e in result.existence.items():
        lines.append(f"{table}: {e.only_run} only in run, {e.only_golden} only in golden, first tick {FormatTick(e.first_tick, e.first_occurrence)} key "
                     f"{FormatKey(table, e.first_key)} ({e.first_side})")
    if result.is_equal:
        lines.append("EQUAL")
    else:
        parts = [f"{len(result.fields)} fields", f"{len(result.existence)} existence", f"{len(result.schema)} schema"]
        if not result.overlap and (result.only_golden_ticks or result.only_run_ticks):
            parts.append(f"{len(result.only_golden_ticks) + len(result.only_run_ticks)} ticks in one file only")
        if not result.ticks_compared:
            parts.append("no common ticks")
        lines.append("DIFFERS: " + ", ".join(parts))
    return lines


# --- approve (D172) --------------------------------------------------------------------------------------------------------------

def GitCommit():
    """Short HEAD of the project, plus "+dirty" when tracked files changed; "unknown" if git fails."""
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    try:
        head = subprocess.run(["git", "-C", root, "rev-parse", "--short", "HEAD"], capture_output=True, text=True, timeout=30)
        status = subprocess.run(["git", "--no-optional-locks", "-C", root, "status", "--porcelain", "--untracked-files=no"],
                                capture_output=True, text=True, timeout=60)
    except (OSError, subprocess.SubprocessError):
        return "unknown"
    if head.returncode != 0 or status.returncode != 0 or not head.stdout.strip():
        return "unknown"
    return head.stdout.strip() + ("+dirty" if status.stdout.strip() else "")


def Approve(run_path, golden_dir, reason, now=None, commit=None):
    """Saves a run as the golden of a film. Everything is validated before a file is touched. Returns the manifest line."""
    reason = (reason or "").strip()
    if not reason or not reason.isascii() or "\n" in reason or "\r" in reason:
        raise TickDumpError("the reason must be one line of ASCII text and not empty.")
    if not os.path.isdir(golden_dir):
        raise TickDumpError(f"{golden_dir} is not a folder.")
    if not os.path.isfile(os.path.join(golden_dir, "film.txt")):
        raise TickDumpError(f"{golden_dir} has no film.txt (write it before approving).")
    run_path = ResolvePath(run_path)
    dump = Dump(run_path)
    name = os.path.basename(run_path)
    if dump.footer is None:
        raise TickDumpError(f"{name} has no footer (the recording was cut).")
    if int(dump.footer["DroppedTicks"]) or any(r.dropped for r in dump.records):
        raise TickDumpError(f"{name} dropped ticks.")
    if int(dump.footer["EventLosses"]) or any(r.flags & EVENTS_LOST for r in dump.records):
        raise TickDumpError(f"{name} lost events.")
    ticks = MatchTicks(dump)
    if not ticks:
        raise TickDumpError(f"{name} has no tick with Match.")
    golden = os.path.join(golden_dir, "golden.tdump")
    if os.path.exists(golden) and os.path.samefile(golden, run_path):
        raise TickDumpError(f"{name} is already the golden.")
    now = now or datetime.datetime.now()
    commit = commit or GitCommit()
    # The .sdump of the same session (M5c, D202) has the name of the run with another extension: it is the input of the replay test.
    run_sdump = os.path.splitext(run_path)[0] + ".sdump"
    has_sdump = os.path.isfile(run_sdump)
    golden_sdump = os.path.join(golden_dir, "golden.sdump")
    shown = f"{name} + {os.path.basename(run_sdump)}" if has_sdump else name
    line = f"{now:%Y-%m-%d %H:%M} | layout 0x{dump.layout_hash:016X} | commit {commit} | ticks {ticks[0]}-{ticks[-1]} | {shown} | {reason}"
    if os.path.exists(golden):
        os.replace(golden, os.path.join(golden_dir, "golden_prev.tdump"))
    shutil.copyfile(run_path, golden)
    if has_sdump:
        shutil.copyfile(run_sdump, golden_sdump)
    elif os.path.exists(golden_sdump):
        # An old golden.sdump would not match the new golden.tdump.
        os.remove(golden_sdump)
    manifest = os.path.join(golden_dir, "manifest.txt")
    previous = b""
    if os.path.exists(manifest):
        with open(manifest, "rb") as handle:
            previous = handle.read()
    with open(manifest, "wb") as handle:
        handle.write((previous + b"\r\n" if previous else b"") + line.encode("ascii"))
    return line


# --- command line ----------------------------------------------------------------------------------------------------------------

def Run(args):
    if args.command == "diff":
        result = Diff(Dump(ResolvePath(args.golden)), Dump(ResolvePath(args.run)), overlap=args.overlap)
        lines = FormatDiff(result, args.max)
        print("\n".join(lines))
        return 0 if result.is_equal else 1
    if args.command == "approve":
        print(Approve(args.run, args.golden_dir, args.reason))
        return 0
    d = Dump(ResolvePath(args.file) if args.file else Newest())
    if args.tick is not None:
        ShowTick(d, args.tick)
    elif args.handle is not None:
        ShowHandle(d, args.handle, args.field)
    else:
        Summary(d)
    return 0


def Main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    show = commands.add_parser("show", help="summary, one tick or one handle of a dump")
    show.add_argument("file", nargs="?")
    show.add_argument("--tick", type=int)
    show.add_argument("--handle", type=lambda s: int(s, 16))
    show.add_argument("--field")
    diff = commands.add_parser("diff", help="compare a run with a golden: exit 0 equal, 1 differs, 2 error")
    diff.add_argument("golden")
    diff.add_argument("run")
    diff.add_argument("--overlap", action="store_true", help="ticks in one file only do not count")
    diff.add_argument("--max", type=int, default=20, help="field lines to print")
    approve = commands.add_parser("approve", help="save a run as the golden of a film")
    approve.add_argument("run")
    approve.add_argument("golden_dir")
    approve.add_argument("--reason", required=True)
    try:
        return Run(parser.parse_args(argv))
    except TickDumpError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(Main())
