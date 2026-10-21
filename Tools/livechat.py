"""
livechat.py
LiveChat with the user while Claude works without ending its turn: the user writes what he sees in Ignore/LiveChat.txt and saves.
Each save is one new message (the file is overwritten, never accumulated). Claude receives it in two ways:
  - Waiting on purpose:   python Tools/livechat.py wait   blocks until the next save (up to --timeout seconds).
  - As an interruption:   the waits of game.py and build.py stop as soon as a message arrives, print it and say how to resume
                          (exit code 4). Nothing running is cancelled: the build or the scenario keeps going.

    python Tools/livechat.py wait [--timeout 170]   Wait for a new message and print it.
    python Tools/livechat.py read                   Print the current message again (marks it as seen).

Exit code: 0 message, 2 no message before --timeout (run the same command again).
State: build/livechat/seen.json (hash of the last message seen). A saved file that is empty or unchanged is not a message.
"""
import argparse, hashlib, json, os, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CHAT = os.path.join(ROOT, "Ignore", "LiveChat.txt")
SEEN = os.path.join(ROOT, "build", "livechat", "seen.json")
INTERRUPTED = 4


def ReadChat():
    try:
        with open(CHAT, "rb") as handle:
            data = handle.read()
    except OSError:
        return ""
    for encoding in ("utf-8-sig", "cp1252"):
        try:
            return data.decode(encoding).strip()
        except UnicodeDecodeError:
            pass
    return data.decode("utf-8", errors="replace").strip()


def Hash(text):
    return hashlib.sha1(text.encode("utf-8")).hexdigest()


def LoadSeen():
    try:
        with open(SEEN, encoding="utf-8") as handle:
            return json.load(handle).get("hash")
    except (OSError, ValueError):
        return None


def MarkSeen(text):
    os.makedirs(os.path.dirname(SEEN), exist_ok=True)
    with open(SEEN + ".tmp", "w", encoding="utf-8") as handle:
        json.dump({"hash": Hash(text), "at": time.strftime("%Y-%m-%d %H:%M:%S")}, handle)
    os.replace(SEEN + ".tmp", SEEN)


def Pending():
    """Returns the new message (and marks it seen) or None. Waits for the file to settle so a save in progress is not cut."""
    text = ReadChat()
    if not text or Hash(text) == LoadSeen():
        return None
    time.sleep(0.5)
    settled = ReadChat()
    if settled != text:
        return None
    MarkSeen(text)
    return text


def Print(text):

    print("LIVECHAT:")
    for line in text.splitlines():
        print("  " + line)


def Interrupt(resume):
    """For the wait loops of other tools: if a message arrived, print it with the command that resumes the wait and exit 4."""
    text = Pending()
    if text is None:
        return
    Print(text)
    print(f"(wait interrupted by LiveChat; nothing was cancelled. Resume: {resume})")
    sys.exit(INTERRUPTED)


def Wait(timeout):
    start = time.time()
    while time.time() - start < timeout:
        text = Pending()
        if text is not None:
            Print(text)
            return 0
        time.sleep(1)
    print(f"livechat: no message in {timeout} s")
    return 2


def Main():
    parser = argparse.ArgumentParser(description="LiveChat with the user through Ignore/LiveChat.txt")
    parser.add_argument("command", choices=["wait", "read"])
    parser.add_argument("--timeout", type=int, default=170)
    args = parser.parse_args()
    if args.command == "read":
        text = ReadChat()
        if text:
            MarkSeen(text)
            Print(text)
        else:
            print("livechat: empty")
        return 0
    return Wait(args.timeout)


if __name__ == "__main__":
    sys.exit(Main())
