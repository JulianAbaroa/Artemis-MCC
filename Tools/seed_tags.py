#!/usr/bin/env python3
"""
seed_tags.py - build / update the object tag catalog of the semantization.

Data/Tags/ObjectTags.xml is the SOURCE OF TRUTH for the Function and Subtype of each object tag
(Ignore/Semantizacion/05, section 3: catalog > MultiplayerObjectKind of the tag > Kind default).
Edit it by hand and commit it. This script only brings in what the game and the old code already know:

  * Dumps   : with a map loaded, the button "Dump object tag catalog of this map" of the
              Definitions Inspector (UI) writes
              %LOCALAPPDATA%\Artemis\TagCatalog\<map>.xml with group, kind and multiplayer object kinds
              of every object tag. Those fields come from the engine and are always refreshed here.
  * Seed    : the hardcoded tag lists of Common.Tag.Type (the old classifier) give a first
              Function/Subtype with confidence="seed".
  * Labels  : function, subtype and confidence are only written while confidence is "unknown"
              or "seed". Anything else (review, low, medium, high) is yours and is never touched.
              "note" holds the evidence or the doubt of a label; the script never writes it.
  * Nothing is deleted, except placeholders of seeds that no longer exist. Maps accumulate in the maps attribute.
  * A tag is identified by group + name: the same name can exist in several groups (a campaign map
    has a "magnum" scenery next to the "magnum" weapon). The seed only applies to the group the old
    classifier expected (SEED_GROUPS).

Usage (from anywhere; defaults resolve relative to the repo):
  python Tools/seed_tags.py "%LOCALAPPDATA%\Artemis\TagCatalog"          # merge every dump of the folder
  python Tools/seed_tags.py "<folder>" --dry-run                    # report only
  python Tools/seed_tags.py --seed-only                             # only apply the Common.Tag.Type seed
"""
import argparse, os, re, sys
import xml.etree.ElementTree as ET

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
CATALOG = os.path.join(REPO, "Data", "Tags", "ObjectTags.xml")
ENUMS = os.path.join(REPO, "Artemis", "Core", "Domain", "2_Resolved", "Definitions", "Type", "Object", "_DefinitionsObject.ixx")
TAGS = os.path.join(REPO, "Artemis", "Core", "Common", "Tag", "Type", "Common.Tag.Type.ixx")

EDITABLE = ("unknown", "seed")
ATTRS = ("name", "group", "kind", "mp", "function", "subtype", "confidence", "maps", "note")

# --- Enums (single source: the C++ definitions) ---

def read_enum(text, name):
    m = re.search(r"enum class %s\s*:\s*[\w:]+\s*\{(.*?)\};" % name, text, re.S)
    if not m:
        sys.exit("Enum %s not found in %s" % (name, ENUMS))
    return {int(v, 16): k for k, v in re.findall(r"(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", m.group(1))}

# --- Seed from Common.Tag.Type ---

# namespace -> (function, {IsFunction: subtype}) ; subtype None = derived from the constant name.
SEED = {
    "Biped": ("Combatant", {"IsSpartan": "Spartan", "IsElite": "Elite"}),
    "ArmorAbility": ("ArmorAbility", None),
    "Ammo": ("AmmoPack", None),
    "Powerup": ("Powerup", None),
    "Spawn": ("SpawnPoint", {"IsInitialSpawn": "Initial", "IsRespawn": "Respawn", "IsInvisibleSpawn": "Invisible"}),
    "Boundary": ("Boundary", None),
    "Explosive": ("Explosive", None),
    "Lift": ("Lift", {"IsVertical": "Vertical", "IsCurved": "Curved", "IsRedirected": "Redirected"}),
    "Shield": ("Barrier", {"IsOneWay": "OneWay", "IsTwoWay": "TwoWay", "IsBlocker": "Blocker", "IsPortableShield": "Portable"}),
    "Teleport": ("Teleporter", {"IsSender": "Sender", "IsReceiver": "Receiver", "IsTwoWay": "TwoWay"}),
    "Weapon": ("Weapon", None),
    "Vehicle": ("Vehicle", None),
    "Pallets": ("Prop", None),
    "ControlDevice": ("HealthStation", None),
    "SkyBox": ("Decor", None),
}
# Constants with their own answer (namespace, constant) -> (function, subtype).
SEED_CONSTANT = {
    ("Objective", "k_FlagStand"): ("ObjectiveSpawn", "FlagStand"),
    ("Objective", "k_CapturePlate"): ("ObjectiveSpawn", "CapturePlate"),
    ("Objective", "k_Flag"): ("ObjectiveItem", "Flag"),
    ("Objective", "k_Bomb"): ("ObjectiveItem", "Bomb"),
    ("Objective", "k_HillMarker"): ("ObjectiveZone", "Hill"),
    ("Shield", "k_DropShield"): ("Barrier", "DropShield"),
    ("ArmorAbility", "k_ArmorLockup"): ("ArmorAbility", "ArmorLock"),
    ("ArmorAbility", "k_ActiveCamouflage"): ("ArmorAbility", "ActiveCamo"),
    ("Vehicle", "k_Shade"): ("Turret", "Shade"),
}
# Groups where the old classifier looked for each function. A seed never labels a tag of another group.
SEED_GROUPS = {
    "Combatant": {"bipd"}, "Vehicle": {"vehi"}, "Turret": {"vehi"}, "Weapon": {"weap"}, "ObjectiveItem": {"weap"},
    "ArmorAbility": {"eqip"}, "Grenade": {"eqip"}, "AmmoPack": {"eqip"}, "Powerup": {"eqip"}, "Explosive": {"bloc"},
    "SpawnPoint": {"scen"}, "Boundary": {"scen"}, "Lift": {"bloc"}, "Barrier": {"bloc", "eqip", "scen"}, "Teleporter": {"bloc"},
    "ObjectiveSpawn": {"bloc"}, "ObjectiveZone": {"bloc"}, "HealthStation": {"ctrl"}, "Prop": {"bloc"}, "Decor": {"scen"},
}
TURRET = re.compile(r"Turret|Cannon|Gun|Chaingun|Gauss|Rocket|Sensor|Grenade")
STRIP = ("Powerup", "Boundary")

def read_seed():
    seeds = {}
    namespace, function, consts = None, None, {}
    for line in open(TAGS, encoding="utf-8"):
        m = re.match(r"^    namespace (\w+)", line)
        if m:
            namespace, consts = m.group(1), {}
            continue
        if namespace is None:
            continue
        m = re.search(r'constexpr const char\* (k_\w+)\{ "(.*)" \}', line)
        if m:
            consts[m.group(1)] = m.group(2).replace("\\\\", "\\")
            continue
        m = re.search(r"constexpr auto (Is\w+|Has\w+)\(", line)
        if m:
            function = m.group(1)
            continue
        if function:
            for const in re.findall(r"k_\w+", line):
                seeds.setdefault((namespace, const), {"tag": consts.get(const), "in": set()})["in"].add(function)
            if line.strip() == "}":
                function = None
        if re.match(r"^    }", line):
            for const, tag in consts.items():
                seeds.setdefault((namespace, const), {"tag": tag, "in": set()})["tag"] = tag
            namespace = None
    out = {}
    for (namespace, const), info in seeds.items():
        tag = info["tag"]
        if not tag:
            continue
        if (namespace, const) in SEED_CONSTANT:
            out[tag] = SEED_CONSTANT[(namespace, const)]
            continue
        if namespace not in SEED:
            continue
        function, by_fn = SEED[namespace]
        name = const[2:]
        for s in STRIP:
            if name.endswith(s) and name != s:
                name = name[: -len(s)]
        subtype = name
        if by_fn is not None:
            subtype = next((by_fn[f] for f in sorted(info["in"]) if f in by_fn), "")
        if namespace == "Weapon" and "IsGrenade" in info["in"]:
            function, subtype = "Grenade", name.replace("Grenade", "")
        elif namespace == "Weapon" and "IsVehicle" in info["in"]:
            function = "Turret"
        elif namespace == "Vehicle" and TURRET.search(name):
            function = "Turret"
        elif namespace in ("Pallets", "ControlDevice", "SkyBox"):
            subtype = ""
        out[tag] = (function, subtype)
    return out

# --- Catalog ---

def load_catalog():
    if not os.path.exists(CATALOG):
        return {}
    root = ET.parse(CATALOG).getroot()
    tags = {}
    for e in root.iter("tag"):
        t = {a: e.get(a, "") for a in ATTRS}
        tags[(t["group"], t["name"])] = t
    return tags

def save_catalog(tags):
    root = ET.Element("catalog", {"game": "Halo: Reach (MCC)"})
    for t in sorted(tags.values(), key=lambda t: (t["group"] or "~", t["name"])):
        ET.SubElement(root, "tag", {a: t[a] for a in ATTRS})
    ET.indent(root, space="\t")
    text = "<?xml version='1.0' encoding='utf-8'?>\n" + ET.tostring(root, encoding="unicode") + "\n"
    os.makedirs(os.path.dirname(CATALOG), exist_ok=True)
    with open(CATALOG, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(text)

def new_tag(group, name):
    return {a: "" for a in ATTRS} | {"name": name, "group": group, "confidence": "unknown"}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dumps", nargs="?", help="folder with the <map>.xml files written by Artemis")
    ap.add_argument("--seed-only", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    if not args.dumps and not args.seed_only:
        ap.error("give the dump folder or --seed-only")

    text = open(ENUMS, encoding="utf-8").read()
    kinds, mps = read_enum(text, "ObjectKind"), read_enum(text, "MultiplayerObjectKind")
    tags = load_catalog()
    added = refreshed = 0
    warnings = []

    if args.dumps:
        files = sorted(f for f in os.listdir(args.dumps) if f.lower().endswith(".xml"))
        if not files:
            sys.exit("No .xml dumps in %s" % args.dumps)
        for f in files:
            root = ET.parse(os.path.join(args.dumps, f)).getroot()
            if root.tag != "map":
                warnings.append("skipped (not a map dump): %s" % f)
                continue
            map_name = root.get("name") or os.path.splitext(f)[0]
            for e in root.iter("tag"):
                name, group = e.get("name"), e.get("group")
                t = tags.get((group, name))
                if t is None:
                    t = tags[(group, name)] = new_tag(group, name)
                    added += 1
                kind = kinds.get(int(e.get("kind")), e.get("kind"))
                mp = ",".join(mps.get(int(v), v) for v in e.get("mp", "").split(",") if v)
                if "," in mp:
                    warnings.append("several multiplayer blocks (first one rules, D56): %s [%s] in %s" % (name, mp, map_name))
                if t["kind"] and (t["kind"], t["mp"]) != (kind, mp):
                    warnings.append("engine data changed: %s %s %s/%s -> %s/%s (%s)" % (group, name, t["kind"], t["mp"], kind, mp, map_name))
                t["kind"], t["mp"] = kind, mp
                maps = set(filter(None, t["maps"].split(";"))) | {map_name}
                t["maps"] = ";".join(sorted(maps))
                refreshed += 1

    # Seed: label the real entries of the expected groups; a tag never seen in a map keeps a placeholder without group.
    seeded = 0
    for tag, (function, subtype) in read_seed().items():
        groups = SEED_GROUPS.get(function, set())
        real = [t for (g, n), t in tags.items() if n == tag and g]
        targets = [t for t in real if t["group"] in groups]
        if real:
            tags.pop(("", tag), None)
        elif ("", tag) not in tags:
            tags[("", tag)] = new_tag("", tag)
            added += 1
        if not real:
            targets = [tags[("", tag)]]
        for t in targets:
            if t["confidence"] in EDITABLE:
                if (t["function"], t["subtype"], t["confidence"]) != (function, subtype, "seed"):
                    seeded += 1
                t["function"], t["subtype"], t["confidence"] = function, subtype, "seed"

    # A placeholder of a seed that no longer exists (the old constant was fixed or removed) is dropped.
    seed_names = set(read_seed())
    for key in [k for k, t in tags.items() if not k[0] and t["confidence"] == "seed" and k[1] not in seed_names]:
        del tags[key]

    unlabeled = sum(1 for t in tags.values() if not t["function"])
    print("Tags: %d (new %d, refreshed %d, seeded %d, without function %d)" % (len(tags), added, refreshed, seeded, unlabeled))
    for w in warnings:
        print("WARNING: " + w)
    if args.dry_run:
        print("Dry run: nothing written.")
        return
    save_catalog(tags)
    print("Written: " + os.path.relpath(CATALOG, REPO))

if __name__ == "__main__":
    main()
