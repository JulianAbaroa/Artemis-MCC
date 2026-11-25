"""
timeline.py
The timeline of each decision in the lockstep (P.3 C, D373): the QPC marks of the tick t (header of the bridge), the marks Python takes while it
reads and answers it, and the Need block of the later tick that says when the game reached t + d and whether it waited. Pure functions, no
Windows and no bridge: CtfEnv with timing keeps the raw marks and env_check --timing prints the table.
"""
import numpy as np

# BridgeNeedFlag bits of NeedFlags.
NEED_WAITED = 1
NEED_TIMED_OUT = 2
NEED_PAUSE_SEEN = 4
NEED_CUT = 8

# Marks of the tick t in order: game thread capture, AI thread out of the tick queue, AI thread done, bridge thread out of its queue, published,
# read by Python (received), answered by Python.
TICK_KEYS = ("capture", "ai_start", "ai_end", "flatten_start", "publish", "received", "answered")

# The stretches of a decision: (name, first mark, last mark, what it is). A stretch with a mark that is 0 (not set) is left out.
SEGMENTS = (
    ("ai_queue", "capture", "ai_start", "tick queue, capture -> AI thread"),
    ("ai", "ai_start", "ai_end", "AI thread"),
    ("bridge_queue", "ai_end", "flatten_start", "bridge queue, AI thread -> flatten"),
    ("flatten", "flatten_start", "publish", "flatten and publish"),
    ("wake", "publish", "received", "Python wakes and copies"),
    ("python", "received", "answered", "Python decides and answers"),
    ("game_to_need", "capture", "need_qpc", "game up to t + d"),
    ("slack", "answered", "need_qpc", "need - answered (negative: the game waited)"),
    ("wait", "need_qpc", "need_end_qpc", "wait of the game thread"),
)


def tick_marks(published) -> dict:
    """The QPC marks a read tick carries (0 when it has none, an older layout or a fake)."""
    return {
        "capture": int(getattr(published, "capture_qpc", 0)),
        "ai_start": int(getattr(published, "ai_start_qpc", 0)),
        "ai_end": int(getattr(published, "ai_end_qpc", 0)),
        "flatten_start": int(getattr(published, "flatten_start_qpc", 0)),
        "publish": int(getattr(published, "publish_qpc", 0)),
        "received": int(getattr(published, "received_qpc", 0)),
    }


def need_marks(published):
    """(decision, need_qpc, need_end_qpc, need_flags) of a read tick, or None when it carries no Need."""
    decision = int(getattr(published, "need_decision", 0))
    if not decision:
        return None
    return decision, int(published.need_qpc), int(published.need_end_qpc), int(published.need_flags)


def join_decisions(decisions: dict, needs: dict) -> list:
    """One row per decision. decisions: key -> marks of the tick plus 'answered'. needs: key -> (need_qpc, need_end_qpc, need_flags), taken from
    a later tick whose NeedDecision is the decision (the key is (match, tick) in both). Rows keep the order of the decisions. A decision without
    a Need has has_need False and the three Need values 0."""
    rows = []
    for key, marks in decisions.items():
        row = dict(marks)
        row["key"] = key
        need = needs.get(key)
        row["has_need"] = need is not None
        row["need_qpc"], row["need_end_qpc"], row["need_flags"] = need if need is not None else (0, 0, 0)
        rows.append(row)
    return rows


def segment_ms(rows: list, frequency: float) -> dict:
    """name -> list of ms of each stretch of SEGMENTS, over the rows that have both marks."""
    result = {}
    for name, first, last, _ in SEGMENTS:
        values = []
        for row in rows:
            a, b = row.get(first, 0), row.get(last, 0)
            if a and b:
                values.append((b - a) * 1000.0 / frequency)
        result[name] = values
    return result


def count_waits(rows: list) -> tuple:
    """(decisions, decisions that waited, decisions without a Need). Waited is the flag of the game thread: a wait that was cut or timed out counts."""
    waited = sum(1 for row in rows if row["has_need"] and row["need_flags"] & NEED_WAITED)
    return len(rows), waited, sum(1 for row in rows if not row["has_need"])


def _percentile(values, p) -> float:
    return float(np.percentile(values, p)) if len(values) else float("nan")


def timeline_lines(rows: list, frequency: float) -> list:
    """The lines of the --timing table of the timeline (ms: p50 / p95 / max per stretch), then how many decisions waited and how many have no Need."""
    if not rows or not frequency:
        return ["timeline: no decision measured"]
    decisions, waited, missing = count_waits(rows)
    lines = [f"timeline over {decisions} decisions (ms: p50 / p95 / max, n = decisions with both marks):"]
    for name, values in segment_ms(rows, frequency).items():
        what = next(text for n, _, _, text in SEGMENTS if n == name)
        peak = max(values) if values else float("nan")
        lines.append(f"  {name:12s} {_percentile(values, 50):8.3f} {_percentile(values, 95):8.3f} {peak:8.3f}  n {len(values):5d}  {what}")
    lines.append(f"  the game waited in {waited} of {decisions} decisions; {missing} have no Need (no tick carried it, or Lockstep was not on)")
    return lines
