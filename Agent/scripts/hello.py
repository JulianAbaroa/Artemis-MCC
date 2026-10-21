"""Smoke test of the Windows venv of Agent (D137): prints the Python and numpy versions."""
import sys
import numpy
print(f"python {sys.version.split()[0]} numpy {numpy.__version__} {sys.executable}")
