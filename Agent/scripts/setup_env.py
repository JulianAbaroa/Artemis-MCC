"""
setup_env.py
Installs Agent/requirements.txt into the venv that runs this script (Agent/.venv, D137). The game watcher installs the requirements
only when it creates the venv; run this after they change.
python Tools/game.py py Agent/scripts/setup_env.py
"""
import os
import subprocess
import sys

REQUIREMENTS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "requirements.txt")


def Main():
    print(f"python {sys.version.split()[0]} at {sys.executable}")
    result = subprocess.run([sys.executable, "-m", "pip", "install", "--disable-pip-version-check", "-q", "-r", REQUIREMENTS])
    if result.returncode != 0:
        print("FAIL pip install")
        return 1
    import torch
    print(f"torch {torch.__version__}, cuda {torch.cuda.is_available()}")
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(Main())
