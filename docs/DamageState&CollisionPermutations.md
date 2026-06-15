# Damage state & collision permutations - Halo Reach (MCC, Update 13)

Technical guide to reproducing, from outside the engine, **which collision
permutation each region of an object is currently showing** as the object takes
damage and dies. The goal is to take a live object, read its runtime damage
state, and decide for every collision mesh whether it should be drawn: intact,
swapped to a damaged variant, swapped to its destroyed variant, or hidden
entirely because the part detached.

The purpose of this document is to capture the model the engine uses, the
`hlmt`<->`coll` relationship, the runtime per-region damage table, and the rules
that turn a damage state into a visible permutation, at the level needed to
implement the system from scratch. It assumes a working `.map` reader (tag
resolution, tag blocks, stringids) and a working collision-geometry reader; see
`MapReader.md` and `MeshReader.md` for those foundations. It also assumes the
reader already knows how to enumerate live objects and read fields off an object
instance.

---

## Credits

The static tag structures (`hlmt`, `coll`) and all field offsets come from
definitions auto-generated from **Assembly** templates
(https://github.com/xboxchaos/assembly). The runtime behaviour, the per-region
damage table layout, the meaning of the damage-level bitmask, and the
death-transition model, was recovered by reverse engineering the retail x64
executable (Ghidra/x64dbg, private PDB symbols) and confirmed with live memory
inspection and hardware breakpoints. Function names of the form `FUN_18xxxxxxx`
are raw addresses from that disassembly and are referenced throughout so the
analysis can be re-walked.

---

## 1. The problem, and why the tag alone is not enough

A vehicle is not a single rigid mesh. Its collision model is split into named
**regions** (hull, fenders, wheels, wings, …), and each region can be drawn in
one of several **permutations** (intact, minor damage, destroyed, …). As the
object takes damage the engine swaps each region's active permutation, and on
death it composes a final "wreck" appearance: some regions swap to a destroyed
mesh, some detach and disappear (spawning separate debris objects), some stay.

Two distinct data sources are involved and both are required:

- The **static tags** (`hlmt`, `coll`) describe *what is possible*: which regions
  exist, which permutations each region has, which damage levels exist, and what
  state each level drives a region into. This never changes at runtime.
- The **live object** carries a small **runtime damage table**: for each damage
  section, how much damage it has taken (a bitmask of crossed levels, plus a
  vitality float). This is what changes moment to moment.

The system is a join: read the runtime table off the object, interpret it
through the static tags, and emit the active permutation per region. Crucially,
**the runtime table does not store "region R is showing permutation P."** It
stores how far each section's damage has progressed; the permutation is *derived*
from that. The whole difficulty is doing that derivation correctly.

---

## 2. The two static structures and how they connect

### 2.1 `coll` — the collision geometry

The `coll` (collision_model) tag holds the geometry the system ultimately draws.
Its relevant shape:

```
coll
└─ Regions[]                 (one per named collision region)
   ├─ Name                   (stringid)
   └─ Permutations[]         (one per variant of that region's geometry)
      ├─ Name                (stringid)
      └─ Bsps[]              (the actual winged-edge collision geometry)
```

So a region is a **named slot**, and under it sit several named **permutations**,
each with its own geometry. "Region 5 = hull, permutations = { default, medium }"
is a typical layout. The default permutation is identified by name (stringid
`0x01`); it is **not** guaranteed to be permutation index 0 in the array, and a
given damage state can map to any permutation index.

### 2.2 `hlmt` — the model, regions, and damage

The `hlmt` (model) tag is the brain. Two of its sub-structures matter:

**Variants → Regions → Permutations → States.** This is the table that answers
"when region R is in damage *state* S, which permutation should it show?" Each
variant lists its regions; each region lists its permutations; each permutation
lists the states under which it is the active one. The state is an enum:

```
Default = 0, Minor = 1, Medium = 2, Major = 3, Destroyed = 4
```

**DamageSections → InstantResponses → RegionTransitions.** This is the table that
answers "as damage accumulates, which *state* does each region move into, and
when?" A damage section is a damageable unit with its own vitality. Each section
has an ordered list of instant responses (thresholds), and each response lists
region transitions (which region goes to which new state).

### 2.3 The join: everything keys on region stringid

`coll` regions, `hlmt` variant regions, and `hlmt` damage-section names are all
**stringids**, and that is the glue. The same stringid (e.g. `0x148B` = "hull")
appears as a `coll` region name, as a variant region name, and as the target of
region transitions. Three independent lookups are built from this:

1. **State → permutation, per region** (`coll` region ⨯ variant): for each `coll`
   region, an array `row[5]` giving the permutation index for each of the five
   states (or `-1` if that state has no geometry). Built by walking the variant's
   regions, matching each to a `coll` region by stringid, then matching each
   state's permutation name to a `coll` permutation name.

2. **Region → damage section** (`coll` region ⨯ damage section name): a damage
   section whose `Name` equals a `coll` region's name *governs* that region. Many
   regions have no matching section, those are **static** (see §4). The body
   section (§5) deliberately does not match any region.

3. **Damage level → state, per region** (the section's own responses): for each
   region that has a governing section, an array mapping damage *level index* to
   the *state* that level drives the region into. Built from the section's
   instant responses (see §6.2).

---

## 3. The runtime damage table on the object

Each live object carries an array of per-damage-section runtime entries. Two
fields on the object locate it:

| Field | Offset | Type | Meaning |
|---|---|---|---|
| Sections count | `0x184` | int16 | number of runtime damage-section entries |
| Sections offset | `0x186` | int16 | offset (from the object base) to the first entry |

Each entry is **`0x18` bytes**. The two fields that matter inside an entry:

| Field | Offset (in entry) | Type | Meaning |
|---|---|---|---|
| Damage-level mask | `0x00` | uint16 | bitfield of crossed damage levels |
| Vitality | `0x10` | float | remaining vitality of this section |

The entry array is indexed by **damage-section slot** (the section's index in the
`hlmt` DamageSections block), **not** by `coll` region index. This is a frequent
and costly mistake: the runtime table and the `coll` region list are different
orderings. The region→section map (§2.3, lookup 2) is what bridges them.

### 3.1 The damage-level mask (offset `0x00`)

This 16-bit field was originally mistaken for an "active permutation index." It
is not. It is a **bitfield where bit N means "damage level N has been crossed."**
Levels are contiguous from bit 0, so the mask progresses
`0x0 → 0x1 → 0x3 → 0x7 → 0xF` as successive thresholds are passed. The
**highest set bit** is the current damage level of that section.

This was confirmed with a hardware write breakpoint on the field, which landed
inside `FUN_1804e0234` at an instruction of the form `*mask |= (1 << level)`,
the engine ORs in the bit for each level it fires.

### 3.2 Object death

Slot 0 is the **body** section (§5). Its vitality is the whole-object health:
**the object is dead when `Sections[0].Vitality <= 0`.** No separate "is dead"
flag is needed; the body's vitality crossing zero is the death signal, and it is
what gates all death-time behaviour (§7).

### 3.3 `HlmtVariant` does not change on death

The active variant index lives on the object at **offset `0xEA`**. It is tempting
to assume death switches the object to a "destroyed variant", it does not. The
destroyed look is just another **permutation** of the hull region within the same
variant. `HlmtVariant` is read once when building the state map and stays put;
nothing about death re-selects it.

---

## 4. Region classes: governed vs. static

Every `coll` region falls into one of two classes, decided by the region→section
map (§2.3, lookup 2):

- **Governed region**: its stringid matches a damage section's name. It has a
  runtime slot; its damage state is read live from the mask/vitality at that
  slot. Fenders, wings, the tank's rollers, etc.
- **Static region**: no matching damage section (slot = `-1`). It has no runtime
  damage of its own and stays at its default permutation throughout the object's
  life. It only ever changes at death, and only because the **body** section
  forces it to (§5, §7). The hull, bumpers, wheels are typically static.

This split matters because the two classes are resolved by different paths: a
governed region derives its state from its own mask; a static region is inert
until death.

---

## 5. The body section and death transitions

Among the damage sections there is always one whose `Name` matches **no** `coll`
region. This is the **body** (slot 0, the section whose vitality is whole-object
health). Its instant responses are special: rather than driving its own region,
they carry **cross-region death transitions**, the list of "when the object
dies, send region X to state Y" instructions.

Concretely, the body's responses at **threshold ≈ 0.0** (the death responses)
contain region transitions targeting many different regions, each with a
`NewState` (almost always `4`, Destroyed). This is the engine's manifest of what
happens to the whole object on death: the hull goes to its destroyed permutation,
wheels are sent to a state they have no geometry for (so they detach), and so on.

The death map is therefore built **only from sections whose name matches no
region** (i.e. the body), reading their threshold-0 responses, and recording per
region the highest `NewState` imposed. A region's own governing section (§4) must
**not** feed this map, those responses drive in-life damage, not death.

The other instant-response flags (`Destroys Object`, `Kills Variant Objects`,
`DestroyInstanceGroupIndex` at offset `0x70`, etc.) were investigated as the
detach-vs-stay signal and found **not** to be a clean discriminator: the same
flag value appears on a region that stays and on one that detaches. The reliable
discriminator is geometric, not flag-based (§7).

---

## 6. Deriving a governed region's state in life

For a governed region, the runtime mask at its slot is turned into a state:

### 6.1 Mask → level

The current damage level is the index of the **highest set bit** of the mask
(`0xF → 3`, `0x7 → 2`, `0x3 → 1`, `0x1 → 0`, `0x0 → none`).

### 6.2 Level → state (cumulative)

The section's instant responses define, per level, the state that level drives
the region into. But two subtleties make a naive `state = levelToState[level]`
wrong:

- **Response order is by descending threshold, not by level.** A section's
  responses are ordered `0.95, 0.5, 0.25, …`; level N corresponds to the Nth
  response in firing order. Build the per-level→state array by walking responses
  in order and taking, for each, its transition that targets the section's own
  region.

- **A level may have no transition for its own region.** Higher levels sometimes
  carry only cross-region effects and leave the region's own state untouched. If
  level N has no own-region transition, the region must **keep the state set by
  the last lower level that did**, damage is cumulative. The correct lookup walks
  *down* from the current level to the highest level that actually defines a
  state, rather than reading the current level's slot blindly. (Reading it
  blindly makes a region "heal" when it crosses into a level with no own
  transition, e.g. a wing visibly popping back intact at `mask = 0x7`.)

The result is the region's in-life state, an index 0–4.

---

## 7. From state to visible permutation

With a state in hand, the active permutation is `row[state]` (§2.3, lookup 1),
with two adjustments.

### 7.1 Clamp / descend over gaps

A region need not have geometry for every state. `row` can be e.g.
`[0, 0, -1, 1, -1]`, states 2 and 4 absent. Two rules:

- **In life**, clamp the state down to the highest state the region actually has
  geometry for (`highestMapped`). A live region is never hidden just because its
  computed state outran its geometry.
- After clamping, if `row[state] == -1`, **descend** (`state--`) until a mapped
  permutation is found. If none is found, fall back to the region's default
  permutation.

### 7.2 Death

On death (`Sections[0].Vitality <= 0`, §3.2), a region may additionally be forced
by the body's death map (§5) to a higher state than its own damage produced. Take
`state = max(ownState, deathState)`. Then, if that exceeds the region's available
geometry (`state > highestMapped`), the engine's intent is one of two things, and
they are told apart **geometrically**:

- **Descend to the highest mapped permutation.** If that permutation is the
  region's **default** (intact) geometry, the region has no real damaged/destroyed
  mesh, it is meant to **detach**, so **hide it** (return false). The engine
  removes it and spawns a separate debris object for it.
- If the landed permutation is a **distinct, non-default** mesh (a genuine damage
  or destroyed variant), the region stays as that **wreck**.

This is the discriminator that flags could not provide: *does the death state
resolve to real damage geometry, or only to the intact mesh?* Intact-only ⇒
detach; distinct mesh ⇒ wreck. It correctly keeps a hull that has a `medium`
mesh as a deformed wreck while detaching wheels that only have an intact mesh.

### 7.3 The unified wreck rule (hull-driven death)

Many vehicles have a hull region with a true **destroyed** permutation
(`row[4] >= 0`). For those, the engine's death appearance is defined *entirely*
by that destroyed mesh, it is the whole wreck silhouette. Every other region is
then redundant (its geometry is either baked into the destroyed hull or
deliberately gone) and must hide.

The rule that captures this: at death, if **any** region of the object has a
destroyed permutation mapped (`row[4] >= 0` for some region), then for each
region:

- if it has its own `row[4] >= 0`, show that (it is part of the wreck);
- otherwise hide it (it is subsumed by the wreck).

If **no** region has a destroyed permutation (e.g. a hull with only
`default`/`medium`), there is no unified wreck; fall back to the per-region
resolution of §7.1–§7.2, and the wreck is the *composition* of each region's
final state.

This single rule resolves the hard cases uniformly: the Falcon (destroyed hull ⇒
propellers, tail, canopy all hide), the Revenant (destroyed front ⇒ rear hides,
engine spawns it as separate debris), the Ghost (destroyed hull including seat),
the Mongoose (destroyed front hull ⇒ rear and wheels hide). The Warthog is the
no-destroyed-hull fallback: its wreck is fenders-in-damage + hull-in-medium +
wheels detached.

---

## 8. Attachments (child objects)

Turrets, side guns, mortars, cannons are **separate objects** with their own
handle and their own `coll`/`hlmt`, parented to the vehicle. They are resolved by
the same per-object logic, with one wrinkle that must be handled explicitly.

### 8.1 Death does not propagate in the data

When the parent vehicle dies, the engine hides/destroys its attachments, but the
**attachment's own body vitality is not driven to zero**. Read in isolation, a
side gun on a dead Falcon still reports `vitality > 0`, i.e. "alive." The runtime
table gives no local signal that it should change.

The relationship is the signal. Walking the parent chain (each object knows its
parent handle), an attachment is treated as dead iff **any ancestor is dead**
(ancestor body vitality `<= 0`). That derived "ancestor dead" is then ORed into
the attachment's own death check, so its body's death responses (§5) fire as if
it had died.

### 8.2 Attachments with no damage geometry

Many attachments have a single permutation per region (default only), so their
entire state map is empty (`row = [-1,-1,-1,-1,-1]`). The "no mapping at all"
shortcut would normally short-circuit to "always show default", which keeps the
turret visible on a dead vehicle. The fix: **before** that shortcut, if the
object is dead (including via ancestor) and the body's death map sends the region
to a destruction state it has no geometry for, hide it. A default-only attachment
on a dead parent thus disappears, which is exactly what the engine shows. An
attachment whose destroyed permutation *is* a distinct mesh instead shows that
mesh, by the same §7 logic, so the behaviour generalizes without special-casing.

---

## 9. The engine functions (disassembly references)

The runtime side was recovered from these functions. They are **not** required to
implement the system, the static tags plus the runtime table at `0x184`/`0x186`
contain everything, but they document where each fact was confirmed and are the
place to re-verify against a different build.

| Function | Role |
|---|---|
| `FUN_1804e0234` | Core damage application. Iterates the section's levels, writes the damage-level mask (`*mask |= 1 << level`), and writes the new state bytes into the region block. The hardware-breakpoint landing site that proved the `0x00` field is a bitmask, not a permutation index. Calls `FUN_1804e08c4` once per fired level. |
| `FUN_1804e08c4` | Per-level consequence materialization. Receives the object handle and the level; resolves the matching instant response and executes its flag-driven effects (transition effects, attachment detach via `DestroyInstanceGroupIndex` at `0x70`, constraints, kill-object). Does **not** itself write the active permutation, and does **not** create debris (no `CreateObject` call); the visible swap is the state already written by `FUN_1804e0234`. Called once per set bit, so the call count per hit is non-constant. |
| `FUN_1804e0800` | Thin dispatcher; only calls `FUN_1804e4c10`. |
| `FUN_1804e4c10` | Spawns the **transition effect** at a marker (markers `0xE6`/`0xED`). Purely visual particles, no geometry/permutation change. |
| `FUN_1804e78a0` | Resolves a region's runtime block: `region_block = *(int16*)(obj + 0x186) + obj + region * 0x18`. The canonical way the engine indexes the `0x18`-stride table. |

The key negative result: hooking these gives no information the runtime table at
`0x184`/`0x186` does not already contain. The level that fires (the call to
`FUN_1804e08c4`) is exactly the set bit in the mask, and the detach-vs-wreck
decision is not written anywhere as a value, it is the *branch the engine takes*,
recoverable statically from the geometry (§7.2). So the system is best built from
the tags + runtime table, not from a hook.

---

## 10. End-to-end resolution per collision mesh

Putting it together, for each collision mesh of an object (each mesh belongs to a
region and is one specific permutation index), decide draw/hide:

1. **Resolve region & slot.** Region index → governing damage-section slot via
   the region→section map. `slot = -1` ⇒ static region.
2. **Compute death once per object.** `dead = Sections[0].Vitality <= 0` OR any
   ancestor dead (§8.1). Note whether any region of this object has a destroyed
   permutation (`hasDestroyedHull`).
3. **If dead and `hasDestroyedHull` (§7.3):** show this mesh iff it is this
   region's `row[4]`; else hide. Done.
4. **Static region (slot −1):** alive ⇒ default permutation. Dead with a
   death-state ⇒ apply §7.2 (show `row[deathState]` if it exists; else descend,
   detach if it lands on default, wreck if on a distinct mesh).
5. **Governed region (slot ≥ 0):** derive state from the mask cumulatively
   (§6). If dead, raise to `max(state, deathState)` and apply the §7.2
   geometric detach/wreck rule. Clamp/descend over gaps (§7.1).
6. **Emit.** The region's resolved permutation index is `wantPerm`; this mesh is
   drawn iff its own permutation index equals `wantPerm`.

---

## 11. Known residuals

Two classes of case are **not** determinable from the readable state and are left
approximate:

- **The "one wheel/roller stays" case** (Warthog, Mongoose, Scorpion). On death
  all wheels report identical state (mask `0x0`, full vitality, same `row`,
  same death-state): they are indistinguishable in every readable field. In game
  one is left attached (and the rest spawn as debris). Which one is a runtime
  decision made at the moment of destruction (tied to the lethal hit / blast
  origin), not stored in any field the table exposes. For a collision/raycast
  use this is negligible and accepted as-is.

- **The detached-in-life small parts** (e.g. Wraith wings) and any attachment
  whose damage data never reaches the runtime table at all (e.g. a turret base
  whose regions never reflect their permutations) are separate data-pipeline
  issues, not failures of this resolution logic.

---

## 12. Reference: key offsets (MCC U13)

| Concept | Where | Offset | Type |
|---|---|---|---|
| Active variant index | object | `0xEA` | (does not change on death) |
| Runtime damage-section count | object | `0x184` | int16 |
| Runtime damage-section offset | object | `0x186` | int16 |
| Runtime section entry stride | — | `0x18` | bytes |
| ↳ damage-level mask | entry | `0x00` | uint16 (bitfield, MSB = level) |
| ↳ section vitality | entry | `0x10` | float (slot 0 = whole-object health) |
| InstantResponse: DamageThreshold | `hlmt` | `0x08` | float (≈0.0 ⇒ death response) |
| InstantResponse: RegionTransitions block | `hlmt` | `0x58` | tagblock |
| InstantResponse: DestroyInstanceGroupIndex | `hlmt` | `0x70` | int16 (not the detach signal) |
| State enum | — | — | Default 0, Minor 1, Medium 2, Major 3, Destroyed 4 |
| Default permutation | `coll` | — | identified by name stringid `0x01` |

All offsets are those of the MCC U13 retail build. Region/permutation/section
identities are all **stringids**; the join across `coll` and `hlmt` is by
stringid equality, never by array position.