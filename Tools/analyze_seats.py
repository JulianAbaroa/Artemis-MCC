"""
analyze_seats.py
Lists the seats of every vehicle tag of a map (A11, Ignore/Tareas/Transversal/A11_Asientos.txt) from the definition dumps of the
DefinitionsInspector (Dump for field-usage report): vehi, hlmt and mode.
For each seat: flags, AI seat kind, boarding (hijack) target, entry radius and angles, the seat marker and the entry markers resolved
in the render model (node, position in node space, +X direction). Ends with a check of the boarding seats against the fixed seat table
of Tables (Artemis/2_Blam/Runtime/Object/Type/Specific/ObjectVehicle.ixx, Layouts).

    python Tools/analyze_seats.py [DUMP_FOLDER]      (default: Ignore/Temporal/forge_all)
"""
import json, math, os, re, sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
D = (sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "Ignore", "Temporal", "forge_all")).rstrip("/\\") + os.sep
TABLE = os.path.join(REPO, "Artemis", "2_Blam", "Runtime", "Object", "Type", "Specific", "ObjectVehicle.ixx")

def load(g):
    path = D + f"definitions_dump_{g}.ndjson"
    if not os.path.exists(path): sys.exit(f"Missing {path}.")
    out = {}
    for l in open(path, encoding="utf-8"):
        j = json.loads(l); out[j["tag"]] = j["fields"]
    return out

vehi, hlmt, mode = load("vehi"), load("hlmt"), load("mode")

SEAT = ["Flags", "Kind", "SeatMarkerName", "EntryMarkerName", "BoardingSeatIndex", "EntryRadius", "EntryMarkerConeAngle",
        "EntryMarkerFacingAngle", "MaximumRelativeVelocity"]
FL = {0: "invisible", 1: "locked", 2: "driver", 3: "gunner", 4: "3rdcam", 5: "weapons", 6: "3rdonenter", 7: "1stslaved",
      8: "needsdriver", 9: "ainoncombat", 10: "boarding", 11: "aifiredisabled", 12: "boardenters", 13: "boardneedpass",
      14: "-player", 15: "-nonplayer"}
KIND = {"0": "None", "1": "Passenger", "2": "Gunner", "3": "SmallCargo", "4": "LargeCargo", "5": "Driver", "255": "Invalid"}

def flags(v):
    v = int(v); return " ".join(FL.get(b, f"b{b}") for b in range(32) if v >> b & 1)

def arr(f, prefix, fields):
    res = []; i = 0
    while f.get(f"{prefix}[{i}].{fields[0]}") is not None:
        res.append({k: f.get(f"{prefix}[{i}].{k}") for k in fields}); i += 1
    return res

def ints(f, prefix):
    res = []; i = 0
    while f.get(f"{prefix}[{i}]") is not None:
        res.append(int(f[f"{prefix}[{i}]"])); i += 1
    return res

def qrot(x, y, z, w, v):
    vx, vy, vz = v
    tx = 2 * (y * vz - z * vy); ty = 2 * (z * vx - x * vz); tz = 2 * (x * vy - y * vx)
    return (vx + w * tx + (y * tz - z * ty), vy + w * ty + (z * tx - x * tz), vz + w * tz + (x * ty - y * tx))

def markers(modename, nameid):
    f = mode.get(modename)
    if not f or nameid in ("0", None): return None
    i = 0
    while f.get(f"MarkerGroups[{i}].NameId") is not None:
        if f[f"MarkerGroups[{i}].NameId"] == nameid:
            res = []
            for m in arr(f, f"MarkerGroups[{i}].Markers", ["NodeIndex", "Translation.X", "Translation.Y", "Translation.Z",
                                                              "Rotation.X", "Rotation.Y", "Rotation.Z", "Rotation.W"]):
                q = [float(m[k]) for k in ["Rotation.X", "Rotation.Y", "Rotation.Z", "Rotation.W"]]
                fwd = qrot(*q, (1, 0, 0))
                pos = tuple(float(m[k]) for k in ["Translation.X", "Translation.Y", "Translation.Z"])
                res.append((int(m["NodeIndex"]), pos, fwd))
            return res
        i += 1
    return []

def fmt(ms):
    if ms is None: return "-"
    if not ms: return "NOT FOUND"
    return "  ".join(f"n{n} ({p[0]:+.2f},{p[1]:+.2f},{p[2]:+.2f}) +X({d[0]:+.2f},{d[1]:+.2f},{d[2]:+.2f})" for n, p, d in ms)

def short(tag): return tag.split("\\")[-1]

seats_by_tag = {}
missing = 0
for tag in sorted(vehi):
    f = vehi[tag]
    model = f.get("Base.ModelTagName", "")
    rm = hlmt.get(model, {}).get("RenderModelTagName", "")
    seats = arr(f, "Seats", SEAT)
    for i, s in enumerate(seats): s["Additional"] = ints(f, f"Seats[{i}].AdditionalBoardingSeats")
    seats_by_tag[tag] = seats
    print(f"\n{tag}  ({len(seats)} seats)  mode {short(rm) if rm in mode else 'MISSING'}")
    for i, s in enumerate(seats):
        sm = markers(rm, s["SeatMarkerName"]); em = markers(rm, s["EntryMarkerName"])
        if sm == [] or em == []: missing += 1
        add = [a for a in s["Additional"] if a >= 0]
        print(f"   {i}: {KIND.get(s['Kind'], s['Kind']):<10} [{flags(s['Flags'])}]  boards {s['BoardingSeatIndex']}"
              f"{' +' + str(add) if add else ''}  entry r {float(s['EntryRadius']):g} cone {math.degrees(float(s['EntryMarkerConeAngle'])):.0f}"
              f" facing {math.degrees(float(s['EntryMarkerFacingAngle'])):.0f} vel {float(s['MaximumRelativeVelocity']):g}")
        print(f"        seat  {fmt(sm)}")
        print(f"        entry {fmt(em)}")

# Check against the fixed table of Tables: hijacker slots must be the boarding seats (H3).
src = open(TABLE, encoding="utf-8").read()
print("\n=== H3: fixed table (Tables Layouts) vs boarding flag of the tag")
names = {"Banshee": "banshee\\banshee", "Falcon": "falcon\\falcon", "FalconTurret": "falcon_side_gun_left",
         "FalconGrenadeTurret": "falcon_side_grenade_left", "Ghost": "ghost\\ghost", "Mongoose": "mongoose\\mongoose",
         "Revenant": "revenant\\revenant", "Scorpion": "scorpion\\scorpion", "ScorpionTurret": "scorpion_anti_infantry",
         "ShadeTurret": "shade\\shade", "Warthog": "warthog\\warthog", "WarthogChaingun": "warthog_chaingun",
         "WarthogGauss": "warthog_gauss", "WarthogRocket": "warthog_rocket", "Wraith": "wraith\\wraith",
         "WraithPlasmaTurret": "wraith_anti_infantry"}
for m in re.finditer(r"Kind::(\w+),\s*\{(.*?)\n\t\t\t\}", src, re.S):
    kind, body = m[1], m[2]
    rows = re.findall(r'\{\s*(\d+),\s*"(\w+)",\s*(true|false),\s*(true|false)\s*\}', body)
    tag = next((t for t in seats_by_tag if t.endswith(names.get(kind, "@@"))), None)
    if not tag:
        print(f"   {kind}: no vehi tag in the dump"); continue
    seats = seats_by_tag[tag]
    bad = []
    for idx, name, hij, slot in rows:
        idx = int(idx)
        if idx >= len(seats): bad.append(f"{name}={idx} out of range"); continue
        boarding = int(seats[idx]["Flags"]) >> 10 & 1 == 1
        if boarding != (slot == "true"): bad.append(f"{name}={idx} boarding={boarding}")
    print(f"   {kind:<20} {short(tag):<24} table {len(rows)} seats, tag {len(seats)}: {'OK' if not bad else 'DIFF ' + ', '.join(bad)}")
print(f"\nSeat or entry marker groups not found in the mode: {missing}")
