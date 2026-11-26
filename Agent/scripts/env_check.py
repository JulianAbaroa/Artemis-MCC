"""
env_check.py
Checks the stage 0 environment against a running CTF match (Diseno/14 sec. 9.2): plays --episodes rounds with a policy and prints,
per episode, steps, return, how it ended, the events of the agents and the time and score left of the observation (from the Match, P.6)
at its start and its end, and how many decisions had the engine's interaction on GrabObjective (the selection reaches the observation).
Policies: random, or seek (steer toward the enemy flag, or the own stand while carrying it, with the vectors of the observation; walk
forward; F5.1 B: while the interaction is GrabObjective and it carries no flag, stop and hold Use: press and hold the first decision,
still held after; otherwise Use on nothing and Jump taps now and then).
python Tools/game.py py Agent/scripts/env_check.py [--policy seek|random|PATH.pt] [--policy2 ...] [--greedy] [--hidden 256] [--episodes 3] [--max-seconds 90] [--scale 5] [--list FILE [--seed 0]] [--timing]
--timing (F4.9): at the end, per phase of a decision (wait for the tick, copy of read_tick, new_events, observe, answer, and the policy, measured here),
p50 / p95 / max in ms and its share of the Python side (copy + events + observe + answer + policy; the wait is not Python's); the slack of the
Lockstep (publish -> answer against the budget of d ticks at the effective scale, scale 0 taken as 1x) and what the counters of Artemis did
per decision (waits, wait per wait, longest wait, flatten per tick).
--timing ends with the timeline of P.3 C (artemis.timeline: per decision, p50 / p95 / max in ms of each stretch from the QPC marks: ai_queue capture -> AI thread,
ai, bridge_queue, flatten, wake = Python wakes and copies, python = decides and answers, game_to_need = the game up to t + d, slack = need - answered
(negative: the game waited), wait = wait of the game thread; and how many decisions made the game wait) and a first line "sparse: granted / not requested".
Since P.3 B the environment reads the Views in place from the shared buffer (copy = only the small blocks) and
validates them with the seqlock; "reread N" counts the ticks read again (0 in Lockstep). --sparse is only a request: when Artemis keeps every tick
(a match with "events" or "tickdump", or a bench scenario without "every_tick": false) take_control raises and this script prints FAIL with the
reason and exits 1, so a measurement never goes on without the sparse pipeline unnoticed.
Exit code 0 when every episode ended (terminated or truncated) and the observations were finite.
Play mode (PL.1): --policy (and --policy2 for the second team) also accept the path of a state_dict of Policy (policy.pt or snapshots/policy_uNNNNN.pt of
train_stage0); nothing is learned. The team with the lowest Team id of the round plays --policy and the other --policy2 (the Team is stable
when the sides swap between rounds); without --policy2 everybody plays --policy. --greedy takes the argmax of each branch instead of sampling (only for
.pt). --episodes 0 plays until Ctrl+C, then prints the summary and releases the control (exit 0). --hidden is the width of the network (PpoConfig.hidden).
A .pt whose observation or action sizes differ from the environment's (other profile or observation) is FAIL, exit 1, with both sizes.
Defaults match train_stage0 (CtfConfig defaults): period 5, latency 5, turn limit 0.1, profile stage0; they differ in --max-seconds (90 here, 60 there),
scale (0 here, 10 there) and --sparse (off here, on there unless --no-sparse).
Example, the video at x1 of the policy of hour 0 against the one of update 2300, greedy, until stopped:
python Tools/game.py py Agent/scripts/env_check.py --policy Agent/runs/hitoa_noche/snapshots/policy_u00000.pt --policy2 Agent/runs/hitoa_noche/snapshots/policy_u02300.pt --greedy --episodes 0 --scale 1
The final summary has: rounds played, terminated / cut, mean and p50 of the seconds of play per round (RoundClockTicks of the Match) and of wall seconds, captures
per agent, rounds won per team and per policy (own vs enemy score left of the last observation; a tie is a draw) and the policy of each agent.
"""
import argparse
import itertools
import math
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import numpy as np  # noqa: E402

from artemis.bridge import CONTROL_STATES, Bridge, discover  # noqa: E402
from artemis.env import PROFILES, ActionNvec, CtfConfig, CtfEnv  # noqa: E402
from artemis.env.ctf import (ACTION_AT, CARRIES_AT, ENEMY_FLAG_AT, GRAB_OBJECTIVE_AT, MOVE_NVEC, OWN_STAND_AT,  # noqa: E402
                             YAW_STEPS)
from artemis.timeline import join_decisions, timeline_lines  # noqa: E402
from artemis.training import load_list  # noqa: E402


def Seek(obs, config, rng):
    is_carrying = obs[CARRIES_AT] > 0.5
    vector = obs[OWN_STAND_AT] if is_carrying else obs[ENEMY_FLAG_AT]
    error = math.atan2(float(vector[1]), float(vector[0]))
    per_decision = min(config.period, config.latency) * config.turn_limit
    yaw_bin = int(np.argmin([abs(step * per_decision - error) for step in YAW_STEPS]))
    names = PROFILES[config.profile]
    is_grabbing = obs[GRAB_OBJECTIVE_AT] > 0.5 and not is_carrying
    # Every control of the profile on "nothing" (state 0) but Jump, which taps now and then (state 1), and Use while grabbing (below).
    states = [1 if name == "Jump" and rng.random() < 0.05 else 0 for name in names]
    forward = 2
    if is_grabbing and "Use" in names:
        # Stand still and hold Use: 2 (press and hold) if the last action had it up, 3 (still held) if it had it down. The last action is
        # read from its one-hot in the observation, so Seek stays a function of the observation.
        use = names.index("Use")
        start = ACTION_AT + sum(MOVE_NVEC) + len(CONTROL_STATES) * use
        last = int(np.argmax(obs[start:start + len(CONTROL_STATES)]))
        states[use] = 3 if last in (2, 3) else 2
        forward = 1
    return np.array([forward, 1, yaw_bin, 2, *states])


def Random(obs, config, rng):
    return np.array([rng.integers(n) for n in ActionNvec(config.profile)])


def LoadPolicy(path, obs_size, nvec, hidden):
    """Policy(obs_size, nvec, hidden) with the state_dict of path (policy.pt format). A file made for other sizes raises ValueError that
    says both: the sizes of the file (from its tensors) and those of the environment."""
    import torch
    from artemis.train.ppo import Policy
    state = torch.load(path, map_location="cpu")
    policy = Policy(obs_size, nvec, hidden)
    try:
        policy.load_state_dict(state)
    except RuntimeError as error:
        have = {k: tuple(v.shape) for k, v in state.items()} if isinstance(state, dict) else {}
        body, actor = have.get("body.0.weight"), have.get("actor.weight")
        found = f"observation {body[1]}, hidden {body[0]}, actions {actor[0]}" if body and actor else "unknown"
        raise ValueError(f"{path}: the policy was saved for {found}, but the environment needs observation {obs_size}, hidden {hidden}, "
                         f"actions {sum(nvec)} (nvec {list(nvec)}): other profile or observation? ({str(error).splitlines()[0]})") from error
    policy.eval()
    return policy


def Learned(policy, greedy):
    """A policy of the (obs, config, rng) form from a Policy: sampled with act() or argmax with greedy()."""
    import torch

    def Act(obs, config, rng):
        with torch.no_grad():
            tensor = torch.as_tensor(obs, dtype=torch.float32)[None]
            actions = policy.greedy(tensor) if greedy else policy.act(tensor)[0]
        return actions[0].numpy()
    return Act


def MakePolicy(name, config, hidden, greedy):
    """(callable, label) of a --policy value: seek, random or the path of a .pt (loaded with the sizes of the environment of config)."""
    if name == "seek":
        return Seek, "seek"
    if name == "random":
        return Random, "random"
    if not os.path.isfile(name):
        raise ValueError(f"--policy {name}: not seek, random or an existing file")
    nvec = ActionNvec(config.profile)
    return Learned(LoadPolicy(name, ACTION_AT + sum(nvec), nvec, hidden), greedy), name


def AssignPolicies(teams, first, second):
    """{handle: name} from {handle: team}: the lowest team plays first and the others second (first for everybody when second is None)."""
    low = min(teams.values()) if teams else None
    return {h: (first if second is None or t == low else second) for h, t in teams.items()}


def RoundWinner(finals, teams):
    """Team that won the round from the last observation of each agent (own / enemy score left in 20 and 21, the same for both teams of a tick):
    the team of an agent that needs less than its enemy. None for a draw, or a variant without a score."""
    for h, obs in finals.items():
        if obs[20] < obs[21] - 1e-9:
            return teams[h]
        if obs[21] < obs[20] - 1e-9:
            return next((t for k, t in teams.items() if t != teams[h]), None)
    return None


def Summarize(rows):
    """Numbers of the final summary from the rows of the rounds. A row: ending, game_seconds, wall_seconds, captures {handle: n},
    winner (a team or None), teams {handle: team}, policies {handle: name}."""
    game = [r["game_seconds"] for r in rows]
    wall = [r["wall_seconds"] for r in rows]
    captures, policies, by_team, by_policy = {}, {}, {}, {}
    for r in rows:
        for h, n in r["captures"].items():
            captures[h] = captures.get(h, 0) + n
        policies.update(r["policies"])
        if r["winner"] is not None:
            by_team[r["winner"]] = by_team.get(r["winner"], 0) + 1
            for h, t in r["teams"].items():
                if t == r["winner"]:
                    by_policy[r["policies"][h]] = by_policy.get(r["policies"][h], 0) + 1
                    break
    return {"rounds": len(rows), "terminated": sum(r["ending"] == "terminated" for r in rows),
            "truncated": sum(r["ending"] == "truncated" for r in rows),
            "game_mean": float(np.mean(game)) if game else float("nan"), "game_p50": Percentile(game, 50),
            "wall_mean": float(np.mean(wall)) if wall else float("nan"), "wall_p50": Percentile(wall, 50),
            "captures": captures, "wins_by_team": by_team, "wins_by_policy": by_policy,
            "draws": sum(r["winner"] is None for r in rows), "policies": policies}


def SummaryLines(summary):
    s = summary
    lines = [f"summary: {s['rounds']} rounds ({s['terminated']} terminated, {s['truncated']} cut, {s['draws']} without a winner)",
             f"  seconds of play per round: mean {s['game_mean']:.1f}, p50 {s['game_p50']:.1f}; wall seconds: mean {s['wall_mean']:.1f}, p50 {s['wall_p50']:.1f}",
             f"  rounds won per team: {dict(sorted(s['wins_by_team'].items()))}; per policy: {s['wins_by_policy']}"]
    for h in sorted(s["policies"]):
        lines.append(f"  agent {h:#010x}: policy {s['policies'][h]}, captures {s['captures'].get(h, 0)}")
    return lines


def Percentile(values, p):
    return float(np.percentile(values, p)) if len(values) else float("nan")


def TimingReport(env, policy_ns, before, after, config):
    """The lines of --timing from the measurements of the environment (CtfConfig.timing), the time of the policy and the counters of the interval."""
    decisions = len(env.timings["wait"])
    if not decisions:
        return ["timing: no decision measured"]
    python = {phase: env.timings[phase] for phase in ("copy", "events", "observe", "answer")}
    python["policy"] = policy_ns
    total = sum(sum(values) for values in python.values()) or 1
    lines = [f"sparse: {'granted' if getattr(env, 'is_sparse', False) else 'not requested'}   reread: {getattr(env, 'reread', 0)}",
             f"timing over {decisions} decisions (ms per decision: p50 / p95 / max, share of the Python side):"]

    def Line(name, values, share):
        ms = [v / 1e6 for v in values]
        text = f"  {name:8s} {Percentile(ms, 50):8.3f} {Percentile(ms, 95):8.3f} {max(ms) if ms else float('nan'):8.3f}"
        return text + (f"  {100 * sum(values) / total:5.1f}%" if share else "  (blocked in wait_tick)")
    lines.append(Line("wait", env.timings["wait"], False))
    for name, values in python.items():
        lines.append(Line(name, values, True))
    budget = config.latency / 60.0 / (config.scale if config.scale > 0 else 1.0) * 1000.0
    slack = env.slack_ms
    over = 100.0 * sum(1 for v in slack if v > budget) / len(slack) if slack else float("nan")
    lines.append(f"  slack publish->answer ms p50 {Percentile(slack, 50):.3f} p95 {Percentile(slack, 95):.3f} max {max(slack) if slack else float('nan'):.3f}; "
                 f"budget d = {config.latency} ticks at scale {config.scale or 1:g} = {budget:.3f} ms; {over:.1f}% of the decisions pass it")
    delta = {k: after[k] - before[k] for k in after}
    waits, published = delta["LockstepWaits"], delta["PublishedTicks"]
    lines.append(f"  counters: waits/decision {waits / decisions:.3f}, wait per wait {(delta['LockstepWaitNs'] / waits / 1e6) if waits else 0:.3f} ms, "
                 f"longest wait {after['LockstepWaitMaxNs'] / 1e6:.3f} ms (since load), flatten per tick {(delta['FlattenNs'] / published / 1e6) if published else 0:.3f} ms")
    lines += timeline_lines(join_decisions(env.decisions, env.needs), getattr(env.bridge, "qpc_frequency", 0))
    return lines


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--policy", default="seek", help="seek, random or the path of a Policy state_dict (.pt)")
    parser.add_argument("--policy2", help="policy of the second team (same values); without it everybody plays --policy")
    parser.add_argument("--greedy", action="store_true", help="argmax of each branch instead of sampling (.pt only)")
    parser.add_argument("--hidden", type=int, help="width of the network of a .pt (default PpoConfig.hidden, 256)")
    parser.add_argument("--episodes", type=int, default=3, help="rounds to play; 0 = until Ctrl+C")
    parser.add_argument("--max-seconds", type=float, default=90.0)
    parser.add_argument("--scale", type=float, default=0.0)
    parser.add_argument("--turn-limit", type=float, default=0.1)
    parser.add_argument("--profile", choices=sorted(PROFILES), default="stage0", help="input profile of the policy (Diseno/22 5.3)")
    parser.add_argument("--sparse", action="store_true", help="ask Artemis for its pipeline only on decisions (default: every tick)")
    parser.add_argument("--list", help="training list (Data/Training/<name>.json): every episode starts with its next setup")
    parser.add_argument("--period", type=int, default=5, help="ticks between decisions (N)")
    parser.add_argument("--latency", type=int, default=5, help="ticks from a decision to its action (d); Artemis clamps it to 2..N (D139)")
    parser.add_argument("--timing", action="store_true", help="measure the phases of each decision and the slack of the Lockstep (F4.9)")
    parser.add_argument("--seed", type=int, default=0, help="seed of a weighted list")
    args = parser.parse_args()
    setups = load_list(args.list, seed=args.seed) if args.list else None
    regions = discover()
    if not regions:
        print("FAIL no open region (is MCC running with Artemis?)")
        return 2
    config = CtfConfig(scale=args.scale, max_seconds=args.max_seconds, turn_limit=args.turn_limit, sparse=args.sparse,
                       profile=args.profile, timing=args.timing,
                       period=args.period, latency=args.latency)
    try:
        hidden = args.hidden
        if hidden is None:
            from artemis.train.ppo import PpoConfig
            hidden = PpoConfig().hidden
        policy_set = {}
        for name in (args.policy, args.policy2):
            if name is not None:
                policy_set[name] = MakePolicy(name, config, hidden, args.greedy)
    except (ValueError, OSError, ImportError) as error:
        print(f"FAIL {error}")
        return 1
    rng = np.random.default_rng(0)
    failures = []
    policy_ns = []
    rows = []
    with Bridge(int(regions[0]["pid"])) as bridge:
        env = CtfEnv(bridge, config)
        try:
            is_taken = env.take_control()
        except RuntimeError as error:
            print(f"FAIL {error}")
            return 1
        if not is_taken:
            print("FAIL Artemis did not hand the instance over")
            return 1
        before = bridge.counters()
        try:
            for episode in (itertools.count() if args.episodes == 0 else range(args.episodes)):
                started = time.time()
                observations, _ = env.reset(setup=setups.next_setup() if setups else None)
                teams = {int(p["Handle"]): int(p["Team"]) for p in env.published.players if int(p["Handle"]) in env.agents}
                names = AssignPolicies(teams, args.policy, args.policy2)
                clock_start = clock_last = int(env.published.match["RoundClockTicks"])
                captures = {h: 0 for h in env.agents}
                returns = {h: 0.0 for h in env.agents}
                events = {h: [] for h in env.agents}
                steps, ending = 0, None
                first = {h: float(o[10]) * 50 for h, o in observations.items()}
                clock = {h: o[19:22].copy() for h, o in observations.items()}
                closest = dict(first)
                grabbing = {h: 0 for h in env.agents}
                while ending is None:
                    decided = time.perf_counter_ns()
                    actions = {h: policy_set[names[h]][0](o, config, rng) for h, o in observations.items()}
                    if args.timing:
                        policy_ns.append(time.perf_counter_ns() - decided)
                    observations, rewards, terminated, truncated, infos = env.step(actions)
                    steps += 1
                    for h in env.agents:
                        grabbing[h] += int(observations[h][GRAB_OBJECTIVE_AT] > 0.5)
                        returns[h] += rewards[h]
                        if observations[h][4] > 0.5 and observations[h][6] < 0.5:
                            closest[h] = min(closest[h], float(observations[h][10]) * 50)
                        events[h] += infos[h]["events"]
                        captures[h] += infos[h]["events"].count("FlagCaptured")
                        if not np.all(np.isfinite(observations[h])):
                            failures.append(f"episode {episode}: observation not finite")
                    clock_last = max(clock_last, int(env.published.match["RoundClockTicks"]))
                    if any(terminated.values()):
                        ending = "terminated"
                    elif any(truncated.values()):
                        ending = "truncated"
                for h in env.agents:
                    print(f"episode {episode} agent {h:#010x}: {steps} steps, return {returns[h]:+.3f}, {ending}, "
                          f"events {events[h]}, flag at {first[h]:.0f} m, closest {closest[h]:.0f} m, GrabObjective in {grabbing[h]} decisions, "
                          f"{time.time() - started:.1f} s")
                    start, end = clock[h], observations[h][19:22]
                    print(f"  policy {names[h]}, team {teams[h]}")
                    print(f"  time left {start[0]:.3f} -> {end[0]:.3f}, score left own/enemy {start[1]:.2f}/{start[2]:.2f} -> "
                          f"{end[1]:.2f}/{end[2]:.2f}")
                rows.append({"ending": ending, "game_seconds": (clock_last - clock_start) / 60.0, "wall_seconds": time.time() - started,
                             "captures": captures, "winner": RoundWinner(observations, teams), "teams": teams,
                             "policies": {h: policy_set[names[h]][1] for h in names}})
        except KeyboardInterrupt:
            print("interrupted: summary and release")
        except (RuntimeError, TimeoutError) as error:
            # The match ended (a team left empty ends it) or the environment gave up waiting: say so and fail, never wait on.
            print(f"FAIL {error}")
            failures.append(str(error))
        finally:
            env.release()
        after = bridge.counters()
    print(f"lost events {bridge.event_gaps}")
    if rows:
        print("\n".join(SummaryLines(Summarize(rows))))
    if args.timing:
        print("\n".join(TimingReport(env, policy_ns, before, after, config)))
    for line in failures[:5]:
        print("  bad:", line)
    print("PASS" if not failures else "FAIL")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(Main())
