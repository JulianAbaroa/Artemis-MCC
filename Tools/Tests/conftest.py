"""
conftest.py
Puts Tools/ (tickdump.py) and this folder (tdump_writer.py) on the import path of the tests.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from tdump_writer import Repo  # noqa: E402

sys.path.insert(0, os.path.join(Repo(), "Tools"))


def pytest_configure(config):
    config.addinivalue_line("markers", "slow: the views pass over the goldens (about two minutes in all); skip it with -m \"not slow\".")
