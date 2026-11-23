"""
control_check.py
The pure part of `bridge_check.py controls` (VW.9c.3 E1, D355, D364): the table of phases of the AgentInput test, the AgentInput of
each phase, the reading of the samples of one player from a published tick, and the checks (PASS / FAIL / SKIP / SEE) of what each
control did, from the samples of the phase and the one before it. It imports nothing from Windows or from the bridge client, so
Tools/Tests runs it without MCC: the runner (bridge_check.py) only talks to the bridge.

A sample is what one decision saw of the agent (Sample). A phase delivers the same AgentInput on every decision for --phase-seconds.
The effect of the input of one decision is seen in the samples of the next decisions.
"""
import math
from dataclasses import dataclass, field

NO_HANDLE = 0xFFFFFFFF
SEATED = 2  # semantic.Binding.Seated: the biped is in a seat of the vehicle its BoundTo names.

# Thresholds (adjustable in K1).
STILL_MOVE = 0.5          # quiet: farthest displacement in m
STILL_TURN = 0.01         # quiet: farthest turn of the aim in rad
STILL_SETTLE = 8          # quiet: decisions skipped first (the body still slides from what it did before; K1 moved 0.92 m)
NO_ZOOM = -1              # Sample.zoom without zoom (the bridge sends 255: ZoomLevel none)
SPRINT = 1                # semantic ArmorAbility.Sprint (Function 6 of the ability entity)
ARMOR_ABILITY = 6         # semantic Function ArmorAbility
MOVE_MIN = 2.0            # forward / left: displacement in the direction asked in m
YAW_TOLERANCE = 0.02      # yaw / pitch: rad each decision may differ from the delta asked
PITCH_TOLERANCE = 0.02
TURN_FRACTION = 0.95      # yaw / pitch: share of the measured decisions inside the tolerance
TURN_WARMUP = 3           # decisions of the phase not measured (the window of the previous decision plays first)
PITCH_STOP = 1.40         # |pitch| from which the engine clamps the look (D358): not a FAIL
CROUCH_FRACTION = 0.9     # CrouchFraction above this counts as crouched
CROUCH_SHARE = 0.5        # crouch: share of the samples that must be crouched
DRAIN_SECONDS = 30.0      # nogren: most time to throw every grenade
COUNTERS = ("RejectedActions", "LockstepTimeouts", "HandleMismatches")


@dataclass
class Sample:
    """What one decision saw of the agent. Plain values, so the tests build them by hand."""
    tick: int = 0
    alive: bool = True
    has_body: bool = True
    pos: tuple = (0.0, 0.0, 0.0)
    yaw: float = 0.0
    pitch: float = 0.0
    grounded: bool = True
    airborne: int = 0
    crouching: bool = False
    crouch_fraction: float = 0.0
    zoom: int = NO_ZOOM
    grenade: int = 0
    frag: int = 0
    plasma: int = 0
    ability: int = NO_HANDLE
    ability_kind: int = 0                 # Subtype of the ability entity (semantic ArmorAbility), 0 unknown
    ability_active: bool = False
    ability_energy: float = 0.0
    equipped: int = NO_HANDLE
    primary: int = NO_HANDLE
    secondary: int = NO_HANDLE
    loaded: int = 0
    reserve: int = 0
    is_energy: bool = False
    heat: float = 0.0
    energy: float = 0.0
    reloading: bool = False
    projectiles: frozenset = frozenset()  # handles of the projectiles of the agent that are not in a hand
    carrying: bool = False                # a flag has the agent as carrier
    vehicle: int = NO_HANDLE              # BoundTo of the biped when Binding = Seated
    vehicle_pos: tuple = (0.0, 0.0, 0.0)
    possessed: bool = False


@dataclass
class Result:
    status: str                            # PASS / FAIL / SKIP / SEE
    detail: str = ""
    notes: list = field(default_factory=list)  # DATO lines: what is not a verdict


@dataclass(frozen=True)
class Pre:
    """What the agent does before a phase starts: controls (name, 'edge' | 'level') for at least `seconds` and `decisions`."""
    controls: tuple
    seconds: float = 0.0
    decisions: int = 0


@dataclass(frozen=True)
class Phase:
    name: str
    check: str
    forward: float = 0.0
    left: float = 0.0
    yaw: float = 0.0
    pitch: float = 0.0
    controls: tuple = ()   # ((control, 'edge' | 'level'), ...): edge = a tap on every decision, level = pressed on the first, held after
    pre: Pre = None
    hint: str = ""         # what the user does by hand in --imitation


def Ctl(name, kind="edge"):
    return ((name, kind),)


FOOT = (
    Phase("quiet", "still", hint="do nothing"),
    Phase("forward", "forward", forward=1.0, hint="W"),
    Phase("left", "left", left=1.0, hint="A"),
    Phase("yaw", "yaw", yaw=0.1, hint="turn the mouse to the left"),
    Phase("pitch_up", "pitch", pitch=0.05, hint="mouse up"),
    Phase("pitch_down", "pitch", pitch=-0.05, hint="mouse down"),
    Phase("jump", "jump", controls=Ctl("Jump"), hint="Space, several times"),
    Phase("crouch", "crouch", controls=Ctl("Crouch", "level"), hint="hold crouch"),
    Phase("fire", "fire", controls=Ctl("Fire", "level"), hint="hold fire"),
    Phase("zoom", "zoom", controls=Ctl("Zoom"), hint="zoom key (toggle)"),
    Phase("zoom_in", "zoom_in", controls=Ctl("ZoomIn"), hint="zoom in key"),
    Phase("zoom_out", "zoom_out", controls=Ctl("ZoomOut"), pre=Pre(Ctl("ZoomIn"), decisions=1), hint="zoom out key"),
    Phase("melee", "see", controls=Ctl("Melee"), hint="melee"),
    Phase("ability", "ability", controls=Ctl("Ability"), hint="ability key"),
    Phase("forward_ability", "forward_ability", forward=1.0, controls=Ctl("Ability", "level"), hint="W and hold the ability"),
    Phase("throw_grenade", "throw", controls=Ctl("ThrowGrenade"), hint="throw grenade"),
    Phase("use_edge", "see", controls=Ctl("Use"), hint="use key (tap)"),
    Phase("use_level", "see", controls=Ctl("Use", "level"), hint="hold the use key"),
    Phase("switch_weapon", "switch_weapon", controls=Ctl("SwitchWeapon"), hint="switch weapon"),
    Phase("switch_grenade", "switch_grenade", controls=Ctl("SwitchGrenade"), hint="switch grenade"),
    Phase("reload", "reload", controls=Ctl("Reload"), pre=Pre(Ctl("Fire", "level"), seconds=1.0), hint="fire a little, then reload"),
)
_BY_NAME = {p.name: p for p in FOOT}

CONTEXTS = {
    "foot": tuple(p.name for p in FOOT),
    "nogren": ("throw_grenade", "switch_grenade", "fire", "melee", "forward"),
    "flag": ("forward", "yaw", "jump", "crouch", "fire", "melee", "use_edge", "use_level", "switch_weapon", "throw_grenade", "ability"),
    "vehicle": ("forward", "yaw", "fire", "zoom", "use_level"),
}


def PhasesOf(context, only=None):
    """The phases of a context in order. `only`: names to keep (exact, or the start of a name before an underscore: 'use' = both).
    A name that matches nothing is a ValueError."""
    phases = [_BY_NAME[name] for name in CONTEXTS[context]]
    wanted = [o.strip() for o in (only or []) if o.strip()]
    if not wanted:
        return phases

    def Matches(phase, o):
        return phase.name == o or phase.name.startswith(o + "_")
    for o in wanted:
        if not any(Matches(p, o) for p in phases):
            raise ValueError(f"no phase '{o}' in the context {context}")
    return [p for p in phases if any(Matches(p, o) for o in wanted)]


def PhaseInput(phase, first, controls):
    """AgentInput of one decision of a phase as a dict (forward, left, yaw_delta, pitch_delta, level, edge). `controls` is
    bridge.CONTROLS (the order of the bits). A level control is pressed (Level 1, Edge 1) on the first decision and still held
    (Level 1, Edge 0) after; an edge one is a tap (Level 0, Edge 1) on every decision. phase None = nothing."""
    if phase is not None:
        return _Input(phase.forward, phase.left, phase.yaw, phase.pitch, phase.controls, first, controls)
    return _Input(0.0, 0.0, 0.0, 0.0, (), first, controls)


def PreInput(pre, first, controls):
    return _Input(0.0, 0.0, 0.0, 0.0, pre.controls, first, controls)


def _Input(forward, left, yaw, pitch, held, first, controls):
    level = edge = 0
    for name, kind in held:
        bit = 1 << controls.index(name)
        if kind == "edge":
            edge |= bit
        else:
            level |= bit
            edge |= bit if first else 0
    return {"forward": forward, "left": left, "yaw_delta": yaw, "pitch_delta": pitch, "level": level, "edge": edge}


def AngleGap(a, b):
    """Distance between two angles in radians, in [0, pi]."""
    return abs((a - b + math.pi) % (2 * math.pi) - math.pi)


def Wrap(angle):
    return (angle + math.pi) % (2 * math.pi) - math.pi


def SampleOf(published, handle):
    """The Sample of the player `handle` in a PublishedTick (arrays of the bridge layout), or None when it is not in the table."""
    rows = published.players[published.players["Handle"] == handle]
    if not len(rows):
        return None
    p = rows[0]
    aim = p["Aim"]
    ax, ay, az = float(aim["X"]), float(aim["Y"]), float(aim["Z"])
    s = Sample(tick=published.action_tick, alive=bool(p["IsAlive"]), has_body=bool(p["HasBody"]),
               yaw=math.atan2(ay, ax), pitch=math.atan2(az, math.hypot(ax, ay)),
               grounded=bool(p["IsGrounded"]), airborne=int(p["AirborneTicks"]), crouching=bool(p["IsCrouching"]),
               crouch_fraction=float(p["CrouchFraction"]), zoom=NO_ZOOM if int(p["Zoom"]) == 255 else int(p["Zoom"]), grenade=int(p["SelectedGrenade"]),
               frag=int(p["FragGrenades"]), plasma=int(p["PlasmaGrenades"]), ability=int(p["Ability"]),
               ability_active=bool(p["IsAbilityActive"]), ability_energy=float(p["AbilityEnergy"]),
               equipped=int(p["EquippedWeapon"]), primary=int(p["PrimaryWeapon"]), secondary=int(p["SecondaryWeapon"]),
               possessed=published.possessed_player == handle)
    ents = published.entities
    gear = ents[ents["Handle"] == s.ability] if s.ability != NO_HANDLE else ents[:0]
    if len(gear) and int(gear[0]["Function"]) == ARMOR_ABILITY:
        s.ability_kind = int(gear[0]["Subtype"])
    body = ents[ents["Handle"] == int(p["BipedHandle"])]
    if len(body):
        b = body[0]
        s.pos = _Vec(b["WorldOrigin"])
        if int(b["Binding"]) == SEATED and int(b["BoundTo"]) != NO_HANDLE:
            s.vehicle = int(b["BoundTo"])
            car = ents[ents["Handle"] == s.vehicle]
            if len(car):
                s.vehicle_pos = _Vec(car[0]["WorldOrigin"])
    weapon = published.weapons[published.weapons["Handle"] == s.equipped]
    if len(weapon):
        w = weapon[0]
        mags = w["Magazines"]
        n = min(int(w["MagazineCount"]), len(mags))
        s.loaded = sum(int(mags["Loaded"][i]) for i in range(n))
        s.reserve = sum(int(mags["Reserve"][i]) for i in range(n))
        s.is_energy = bool(w["IsEnergyWeapon"])
        s.heat = _Finite(w["Heat"])
        s.energy = _Finite(w["Energy"])
        s.reloading = bool(w["IsReloading"])
    shots = published.projectiles
    s.projectiles = frozenset(int(h) for h in shots["Handle"][(shots["OwnerPlayer"] == handle) & (shots["InHand"] == 0)])
    s.carrying = bool((published.objectives["CarrierPlayer"] == handle).any())
    return s


def _Vec(v):
    return (float(v["X"]), float(v["Y"]), float(v["Z"]))


def _Finite(value):
    value = float(value)
    return value if math.isfinite(value) else 0.0


def Grenades(s):
    return s.frag + s.plasma


def NoWeapon(handle):
    return handle in (0, NO_HANDLE)


# ---------------------------------------------------------------------------------------------------------------- checks

def _Dist(a, b):
    return math.dist(a, b)


def _Proj(before, last, left=False):
    """Displacement from the start of the phase projected on the look of the start (forward) or on its left, in m."""
    dx, dy = last.pos[0] - before.pos[0], last.pos[1] - before.pos[1]
    c, s = math.cos(before.yaw), math.sin(before.yaw)
    return dx * -s + dy * c if left else dx * c + dy * s


def _Still(phase, samples, before, extras):
    if len(samples) > STILL_SETTLE:
        before, samples = samples[STILL_SETTLE - 1], samples[STILL_SETTLE:]
    move = max(_Dist(s.pos, before.pos) for s in samples)
    turn = max(AngleGap(s.yaw, before.yaw) for s in samples)
    detail = f"moved {move:.2f} m, turned {turn:.3f} rad"
    return Result("PASS" if move < STILL_MOVE and turn < STILL_TURN else "FAIL", detail)


def _Walk(phase, samples, before, left=False):
    far = max(_Proj(before, s, left) for s in samples)
    detail = f"{far:.2f} m {'left' if left else 'forward'} (need > {MOVE_MIN})"
    return Result("PASS" if far > MOVE_MIN else "FAIL", detail)


def _Forward(phase, samples, before, extras):
    return _Walk(phase, samples, before)


def _Left(phase, samples, before, extras):
    return _Walk(phase, samples, before, left=True)


def _Turn(phase, samples, before, extras, axis):
    asked = phase.yaw if axis == "yaw" else phase.pitch
    tol = YAW_TOLERANCE if axis == "yaw" else PITCH_TOLERANCE
    chain = [before] + list(samples)
    ok = total = capped = 0
    for i in range(1 + TURN_WARMUP, len(chain)):
        a, b = chain[i - 1], chain[i]
        if axis == "pitch" and (abs(a.pitch) >= PITCH_STOP or abs(b.pitch) >= PITCH_STOP):
            capped += 1
            continue
        turned = Wrap(b.yaw - a.yaw) if axis == "yaw" else b.pitch - a.pitch
        total += 1
        ok += (AngleGap(turned, asked) if axis == "yaw" else abs(turned - asked)) < tol
    if not total:
        if capped:
            return Result("PASS", f"the look reached the engine stop (|pitch| >= {PITCH_STOP}) before {TURN_WARMUP + 1} measurable decisions")
        return Result("SKIP", "too few decisions to measure")
    detail = f"{ok}/{total} decisions turned {asked:+.2f} +- {tol}" + (f", {capped} at the stop" if capped else "")
    return Result("PASS" if ok >= TURN_FRACTION * total else "FAIL", detail)


def _Yaw(phase, samples, before, extras):
    return _Turn(phase, samples, before, extras, "yaw")


def _Pitch(phase, samples, before, extras):
    return _Turn(phase, samples, before, extras, "pitch")


def _Jump(phase, samples, before, extras):
    air = any(s.airborne > 0 or not s.grounded for s in samples)
    chain = [before] + list(samples)
    jumps = sum(1 for a, b in zip(chain, chain[1:]) if a.grounded and not b.grounded)
    return Result("PASS" if air else "FAIL", f"{jumps} jumps (ground flanks), airborne samples {sum(1 for s in samples if s.airborne > 0 or not s.grounded)}")


def _Crouch(phase, samples, before, extras):
    n = sum(1 for s in samples if s.crouching or s.crouch_fraction > CROUCH_FRACTION)
    return Result("PASS" if n > CROUCH_SHARE * len(samples) else "FAIL", f"crouched in {n}/{len(samples)} samples")


def _Fire(phase, samples, before, extras):
    if NoWeapon(before.equipped):
        return Result("SKIP", "no weapon equipped")
    if before.is_energy:
        heat = max(s.heat for s in samples) - before.heat
        drop = before.energy - min(s.energy for s in samples)
        return Result("PASS" if heat > 0 or drop > 0 else "FAIL", f"energy weapon: heat +{heat:.3f}, energy -{drop:.3f}")
    if before.loaded == 0:
        return Result("SKIP", "magazine empty before the phase")
    drop = before.loaded - min(s.loaded for s in samples)
    return Result("PASS" if drop > 0 else "FAIL", f"magazine {before.loaded} -> {min(s.loaded for s in samples)}")


def _Zoom(phase, samples, before, extras):
    top = max(s.zoom for s in samples)
    if top == before.zoom and all(s.zoom == before.zoom for s in samples):
        return Result("SKIP", f"Zoom never changed ({before.zoom}): the weapon has no zoom")
    return Result("PASS" if top > NO_ZOOM else "FAIL", f"Zoom {before.zoom} -> up to {top}")


def _ZoomIn(phase, samples, before, extras):
    top = max(s.zoom for s in samples)
    if top == before.zoom and all(s.zoom == before.zoom for s in samples):
        return Result("SKIP", f"Zoom never changed ({before.zoom}): the weapon has no zoom")
    return Result("PASS" if top > before.zoom else "FAIL", f"Zoom {before.zoom} -> up to {top}")


def _ZoomOut(phase, samples, before, extras):
    if before.zoom == NO_ZOOM:
        return Result("SKIP", "no zoom to take out (the Zoom of the start did not turn it on: the weapon has no zoom)")
    low = min(s.zoom for s in samples)
    return Result("PASS" if low == NO_ZOOM else "FAIL", f"Zoom {before.zoom} -> down to {low}")


def _Ability(phase, samples, before, extras):
    # note: sprint only runs while moving (K1: tap standing still = inactive); forward_ability covers it.
    if phase.forward == 0 and before.ability_kind == SPRINT:
        return Result("SKIP", "sprint only runs while moving: forward_ability covers it")
    if before.ability in (0, NO_HANDLE) and not before.ability_energy and not any(s.ability_active for s in samples):
        return Result("SKIP", "no ability")
    active = any(s.ability_active for s in samples)
    drop = before.ability_energy - min(s.ability_energy for s in samples)
    return Result("PASS" if active or drop > 0 else "FAIL", f"active {active}, energy -{drop:.3f}")


def _ForwardAbility(phase, samples, before, extras):
    walk, ability = _Forward(phase, samples, before, extras), _Ability(phase, samples, before, extras)
    detail = f"forward: {walk.detail}; ability: {ability.detail}"
    if "FAIL" in (walk.status, ability.status):
        return Result("FAIL", detail)
    return Result("SKIP" if ability.status == "SKIP" else "PASS", detail)


def _NewShots(samples, before):
    new = set()
    for s in samples:
        new |= s.projectiles - before.projectiles
    return new


def _Throw(phase, samples, before, extras):
    if Grenades(before) == 0:
        return Result("SKIP", "no grenades")
    new = _NewShots(samples, before)
    low = min(Grenades(s) for s in samples)
    return Result("PASS" if low < Grenades(before) or new else "FAIL", f"grenades {Grenades(before)} -> {low}, {len(new)} projectiles")


def _NoGrenThrow(phase, samples, before, extras):
    new = _NewShots(samples, before)
    grew = max(Grenades(s) for s in samples) > 0
    return Result("FAIL" if new or grew else "PASS", f"no grenades: {len(new)} new projectiles of the agent, grenades {'appeared' if grew else 'stay 0'}")


def _NoGrenSwitch(phase, samples, before, extras):
    same = all(s.grenade == before.grenade for s in samples)
    return Result("PASS" if same else "FAIL", f"SelectedGrenade {before.grenade}" + ("" if same else " changed"))


def _SwitchWeapon(phase, samples, before, extras):
    if NoWeapon(before.primary) or NoWeapon(before.secondary):
        return Result("SKIP", "only one weapon")
    changed = any(s.equipped != before.equipped for s in samples)
    return Result("PASS" if changed else "FAIL", "EquippedWeapon " + ("changed" if changed else "did not change"))


def _SwitchGrenade(phase, samples, before, extras):
    if (before.frag > 0) + (before.plasma > 0) < 2:
        return Result("SKIP", "fewer than two grenade types with grenades")
    changed = any(s.grenade != before.grenade for s in samples)
    return Result("PASS" if changed else "FAIL", f"SelectedGrenade {before.grenade} " + ("changed" if changed else "did not change"))


def _Reload(phase, samples, before, extras):
    if before.reserve == 0:
        return Result("SKIP", "Reserve 0")
    pre = extras.get("pre") or []
    if pre and min(s.loaded for s in pre) >= pre[0].loaded and not before.reloading:
        return Result("SKIP", "the magazine did not go down with the fire of the start")
    reloading = any(s.reloading for s in samples)
    rose = max(s.loaded for s in samples) > before.loaded
    return Result("PASS" if reloading or rose else "FAIL", f"IsReloading {reloading}, Loaded {before.loaded} -> {max(s.loaded for s in samples)}")


def _See(phase, samples, before, extras):
    return Result("SEE", "no field of the Tick proves it: the user confirms it by LiveChat")


def _VehicleForward(phase, samples, before, extras):
    if before.vehicle == NO_HANDLE:
        return Result("SKIP", "the agent is not in a vehicle")
    far = max(_Dist(s.vehicle_pos, before.vehicle_pos) for s in samples)
    return Result("PASS" if far > MOVE_MIN else "FAIL", f"vehicle {before.vehicle:#x} moved {far:.2f} m (need > {MOVE_MIN})")


CHECKS = {"still": _Still, "forward": _Forward, "left": _Left, "yaw": _Yaw, "pitch": _Pitch, "jump": _Jump, "crouch": _Crouch,
          "fire": _Fire, "zoom": _Zoom, "zoom_in": _ZoomIn, "zoom_out": _ZoomOut, "ability": _Ability,
          "forward_ability": _ForwardAbility, "throw": _Throw, "switch_weapon": _SwitchWeapon, "switch_grenade": _SwitchGrenade,
          "reload": _Reload, "see": _See, "nogren_throw": _NoGrenThrow, "nogren_switch": _NoGrenSwitch,
          "vehicle_forward": _VehicleForward}


def CheckName(phase, context):
    """The check of a phase in a context."""
    if context == "nogren" and phase.check in ("throw", "switch_grenade"):
        return "nogren_throw" if phase.check == "throw" else "nogren_switch"
    if context == "vehicle":
        return "vehicle_forward" if phase.name == "forward" else "see"
    return phase.check


def CheckPhase(phase, samples, before, context="foot", extras=None):
    """Verdict of one phase. samples: the Samples of its decisions after the first; before: the one of the first decision (what the
    agent was like when the phase started). Dying is SKIP, not a FAIL of the control. In the flag context a lost flag is a note."""
    extras = extras or {}
    if before is None or not samples:
        return Result("SKIP", "no samples")
    if not (before.alive and before.has_body) or any(not (s.alive and s.has_body) for s in samples):
        return Result("SKIP", "died")
    result = CHECKS[CheckName(phase, context)](phase, samples, before, extras)
    if context == "flag" and before.carrying:
        lost = next((i for i, s in enumerate(samples) if not s.carrying), None)
        if lost is not None:
            result.notes.append(f"DATO: the agent stopped carrying the flag at decision {lost + 1} of {len(samples)} (control: {phase.name})")
    if before.primary != samples[-1].primary or before.secondary != samples[-1].secondary:
        result.notes.append(f"DATO: weapons changed: primary {before.primary:#x} -> {samples[-1].primary:#x}, "
                            f"secondary {before.secondary:#x} -> {samples[-1].secondary:#x}")
    return result


def GrewCounters(before, after):
    """Names of the bridge counters that must not grow (rejected actions, lockstep timeouts, handle mismatches) and did."""
    return [name for name in COUNTERS if after[name] > before[name]]


# ------------------------------------------------------------------------------------------------------------- imitation

def WindowOf(published):
    """The PossessedInput of a PublishedTick as a dict (forward, left, yaw_delta, pitch_delta, level, edge), or None when nobody
    possesses a player."""
    if published.possessed_player == NO_HANDLE:
        return None
    pi = published.possessed_input
    return {"forward": float(pi["MoveForward"]), "left": float(pi["MoveLeft"]), "yaw_delta": float(pi["YawDelta"]),
            "pitch_delta": float(pi["PitchDelta"]), "level": int(pi["Level"]), "edge": int(pi["Edge"])}


def CheckImitation(phase, windows, controls):
    """Verdict of one phase of --imitation. windows: the PossessedInput of the closed windows seen during the phase, as dicts
    (forward, left, yaw_delta, pitch_delta, level, edge), None where nobody possessed the bot. controls: bridge.CONTROLS."""
    seen = [w for w in windows if w is not None]
    if not seen:
        return Result("SKIP", "no possession")
    level = edge = 0
    for w in seen:
        level |= int(w["level"])
        edge |= int(w["edge"])
    fwd, left = sum(w["forward"] for w in seen), sum(w["left"] for w in seen)
    yaw, pitch = sum(w["yaw_delta"] for w in seen), sum(w["pitch_delta"] for w in seen)
    problems, mine = [], 0
    for name, kind in phase.controls:
        bit = 1 << controls.index(name)
        mine |= bit
        if not edge & bit:
            problems.append(f"{name} missing in Edge")
        if kind == "level" and not level & bit:
            problems.append(f"{name} missing in Level")
    for label, total, asked in (("forward", fwd, phase.forward), ("left", left, phase.left), ("yaw", yaw, phase.yaw), ("pitch", pitch, phase.pitch)):
        if asked and total * asked <= 0:
            problems.append(f"{label} sum {total:+.3f} has not the sign asked ({asked:+.2f})")
    others = [name for i, name in enumerate(controls) if ((level | edge) >> i) & 1 and not mine >> i & 1]
    notes = []
    if others:
        notes.append("DATO: other bits seen: " + ", ".join(others))
    extra_axes = [f"{label} {total:+.3f}" for label, total, asked in (("forward", fwd, phase.forward), ("left", left, phase.left),
                  ("yaw", yaw, phase.yaw), ("pitch", pitch, phase.pitch)) if not asked and abs(total) > 1e-6]
    if extra_axes:
        notes.append("DATO: other axes: " + ", ".join(extra_axes))
    detail = f"{len(seen)}/{len(windows)} windows, Level {level:#x} Edge {edge:#x}, forward {fwd:+.2f} left {left:+.2f} yaw {yaw:+.2f} pitch {pitch:+.2f}"
    if phase.check == "still":
        return Result("PASS" if not (level or edge or extra_axes) else "SEE", detail, notes)
    if problems:
        return Result("FAIL", detail + "; " + ", ".join(problems), notes)
    return Result("PASS", detail, notes)


def Summarize(results):
    """{status: count} of a list of Results."""
    counts = {"PASS": 0, "FAIL": 0, "SKIP": 0, "SEE": 0}
    for r in results:
        counts[r.status] += 1
    return counts


def FormatLine(context, phase_name, result):
    line = f"{context:8s} {phase_name:16s} {result.status:4s} {result.detail}"
    return "\n".join([line] + [f"{'':26s}{n}" for n in result.notes])
