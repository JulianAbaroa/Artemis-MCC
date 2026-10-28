"""
components.py
The C++ projects of Artemis in the order of the solution (D229): one source of truth for the tools that need their folders. The
solution folders of Artemis-MCC.slnx and the folders on disk mirror each other: project Artemis.<Name> lives in
Artemis/<Group>/<Folder>/Artemis.<Name>.vcxproj. No group depends on a later one; inside a group the alphabetical order of the folders
is also an order of dependencies (test_components checks both). Artemis/External/ is the only folder outside this table.
"""

# (group, folder, project name without "Artemis.")
PROJECTS = (
    ("1_Shared", "Common", "Common"),
    ("1_Shared", "Contract", "Contract"),
    ("1_Shared", "Services", "Services"),
    ("2_Blam", "Layout", "Blam.Layout"),
    ("2_Blam", "Map", "Blam.Map"),
    ("2_Blam", "MapModel", "Blam.MapModel"),
    ("2_Blam", "Runtime", "Blam.Runtime"),
    ("3_Domain", "Control", "Control"),
    ("3_Domain", "Perception", "Perception"),
    ("3_Domain", "Views", "Views"),
    ("4_Bridge", "Codec", "Bridge.Codec"),
    ("4_Bridge", "Transport", "Bridge.Transport"),
    ("5_Engine", "Base", "Engine.Base"),
    ("5_Engine", "Capture", "Engine.Capture"),
    ("5_Engine", "Control", "Engine.Control"),
    ("5_Engine", "Match", "Engine.Match"),
    ("5_Engine", "Render", "Engine.Render"),
    ("6_DevTools", "DevTools", "DevTools"),
    ("7_Tests", "GoogleTest", "GoogleTest"),
    ("7_Tests", "Tests", "Tests"),
    ("8_Mod", "Mod", "Mod"),
)
SOLUTION = "Artemis-MCC.slnx"


def Folder(name):
    """Repo-relative folder of a project ("Blam.Map" -> "Artemis/2_Blam/Map")."""
    for group, folder, project in PROJECTS:
        if project == name:
            return f"Artemis/{group}/{folder}"
    raise KeyError(name)


def Project(name):
    """Repo-relative path of the .vcxproj of a project ("Blam.Map" -> "Artemis/2_Blam/Map/Artemis.Blam.Map.vcxproj")."""
    return f"{Folder(name)}/Artemis.{name}.vcxproj"


def Short(path):
    """Path for messages: without the leading "Artemis/"."""
    return path[len("Artemis/"):] if path.startswith("Artemis/") else path
