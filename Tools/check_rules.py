"""
check_rules.py
Checks the C++ code of Artemis against the mechanical rules of Ignore/rules_cpp.txt, file by file and across the project.
Each finding has the id of its check (see CHECKS below) and points to a file and a line.

    python Tools/check_rules.py                   New findings only (the ones not in the baseline). Exit code 1 if there are any.
    python Tools/check_rules.py --all             Every finding, baseline included.
    python Tools/check_rules.py --summary         Count per check (new / baseline).
    python Tools/check_rules.py --files A.ixx B.cpp   Only these files (project checks still read every file).
    python Tools/check_rules.py --only S1,U1      Only these checks.
    python Tools/check_rules.py --update-baseline Accept every current finding (Tools/check_rules_baseline.txt).
    python Tools/check_rules.py --self-test       Checks the checker: Tools/check_rules_cases/ has one violation of each file rule.
    python Tools/check_rules.py --dead            Possibly dead methods (X1). Informative, never fails.

The baseline holds the findings that already existed when a check was added, so only new code has to be clean. A finding is
matched by file, check and the text of its line (not the line number), so moving code does not make it new. When old code is
fixed, --update-baseline shrinks the baseline.
Generated files (TagTranspiler, GenerateDatum, GenerateSemantic) only get the project checks.
What is not mechanical (comments that explain the contract, minimal changes, dead code, where a design belongs) is for the reviewer.
"""
import argparse, collections, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE = os.path.join(ROOT, "Tools", "check_rules_baseline.txt")
# Folders with C++ code of the DLL and its libraries. Each one has its own Visual Studio project (P1): Core is part of Artemis.vcxproj.
SOURCE_ROOTS = ("Artemis/Core/", "Artemis/Common/", "Artemis/Services/", "Artemis/Blam.Map/", "Artemis/Blam.MapModel/", "Artemis/Blam.Layout/", "Artemis/Blam.Runtime/")
PROJECT_OF = {
    "Artemis/Core/": "Artemis/Artemis",
    "Artemis/Common/": "Artemis/Common/Artemis.Common",
    "Artemis/Services/": "Artemis/Services/Artemis.Services",
    "Artemis/Blam.Map/": "Artemis/Blam.Map/Artemis.Blam.Map",
    "Artemis/Blam.MapModel/": "Artemis/Blam.MapModel/Artemis.Blam.MapModel",
    "Artemis/Blam.Layout/": "Artemis/Blam.Layout/Artemis.Blam.Layout",
    "Artemis/Blam.Runtime/": "Artemis/Blam.Runtime/Artemis.Blam.Runtime",
}

CHECKS = {
    # Files (rules 6)
    "F1": "Tab character (4 spaces, never tabs).",
    "F2": "Trailing whitespace.",
    "F3": "Line ending is not CRLF.",
    "F4": "File ends with a newline (no final newline in C++ files).",
    # Style (rules 4)
    "S1": "Function without trailing return syntax (auto Name(...) -> Return).",
    "S2": "Member or field without '{}' initializer.",
    "S3": "Brace not on its own line (Allman).",
    "S4": "Own method called without 'this->'.",
    # Naming (rules 4)
    "N1": "Class data member not named m_Pascal.",
    "N2": "Constant not named k_Pascal.",
    "N3": "Static member not named s_Pascal.",
    "N4": "Bool member not named m_Is<Name>.",
    "N5": "Bool getter named Get/Should/Must (use Is<Name>).",
    # Usings and types (rules 4)
    "U1": "First private scope of a class holds something other than 'using X = A::B::X;'.",
    "U2": "Type renamed with 'using X = Y' (one name per type).",
    "U3": "Anonymous namespace uses 'using X = A::B::X' (use 'using A::B::X;').",
    "U4": "Constant written with its full namespace chain (bring it with 'using' and write k_Name).",
    "U5": "Type of another namespace brought into a named namespace (it is exported with it: use the anonymous namespace).",
    "T1": "struct/enum defined outside a Type module (or inside a class).",
    # Modules and imports (rules 4)
    "M1": ".cpp imports a partition of its own module by full name (use 'module A.B.C;' then 'import :Part;').",
    "M2": "'import std;' is not the last import.",
    "M3": "Imports not ordered by layer (Common, Service, Platform, Domain 1-8, Gui, Viewer, UI, Runtime).",
    # Comments and logs (rules 4)
    "C1": "Block comment '/* */' (only '//').",
    "C2": "';' inside a comment (split into sentences).",
    "L1": "Log text not in the form \"[ClassName] LEVEL: text.\" (LEVEL = ERROR, WARNING, INFO).",
    # Hooks (rules 4)
    "H1": "Hook static defined in the .cpp (declare and initialize it in the class: static inline ... { nullptr }).",
    # Project (rules 2 and 3)
    "A1": "Import goes up the layers (a layer only imports lower layers).",
    "D1": "Same constant (name and value) defined in more than one file.",
    "D2": "Same engine value under different constant names in different files.",
    # Added with deps.py / refs.py (rules 2, 3, 4, 6)
    "A2": "Singleton outside a hook (static s_Instance only in Hook modules).",
    "A3": "Specialization imports go up (Type imports only Type, State only Type and State).",
    "M4": "Third level of the module name is not Type, State, System or Hook.",
    "M5": "Namespace does not match the module name (+1 level for a Type partition).",
    "W1": "std::max / std::min without parentheses (windows.h defines max: write (std::max)).",
    "N6": "Hungarian prefix in a parameter or local (pSwapChain -> swapChain).",
    "C3": "Comment cites an Ignore/ path (cite the decision id, D97).",
    "C6": "Comment starts with 'This function/method/class...' (verb phrase).",
    "C7": "Comment not in English.",
    "H2": "Function pointer type outside a Type module, not named <Name>Function, or typedef.",
    "H3": "Aggregating .ixx of a Hook module inside Hook/ (it lives in the folder of the specialization).",
    "H4": "Hooked function without in-flight count (InFlightScope / HookScope).",
    "O1": "Raw hex value in a System, Hook or .cpp body (engine offsets, RVAs, bits, masks and opcodes only in Type modules).",
    "Y1": "Layer groups not in order '// --- State ---', '// --- System ---', '// --- Hook ---'.",
    "Y2": "File layout: blank lines around the module line and imports, 'module;' only with includes.",
    "Y3": "Class sections not in order [private: usings] public: [private:].",
    "P1": "Source not listed in Artemis.vcxproj / .filters / Artemis-MCC.pyproj.",
    "X1": "Possibly dead method: declared, name used nowhere else (only with --dead, does not fail).",
}

# Rank of each module prefix. A module may import modules of its own rank or lower (A1), and imports go in rank order (M3).
LAYER_RANK = {
    "Common": 0, "Service": 1, "Platform": 2,
    "Map": 10, "Resolved": 11, "Datum": 12, "Layout": 12, "Tables": 13, "Relations": 14, "Environment": 15, "Egocentric": 16, "Export": 17,
    "Gui": 20, "Viewer": 21, "UI": 22, "Runtime": 23, "Core": 30,
}
# Modules of Blam.Layout (the format of the engine memory as data): any layer may import them, they sit below all of them.
LIBRARY_MODULES = ("Datum", "Layout")
SPECIALIZATION_ORDER = {"Type": 0, "State": 1, "System": 2, "Hook": 3}
GENERATED_MARKERS = ("auto-generated", "generated by", "do not edit")
SPECIFIERS = r"(?:(?:static|inline|virtual|explicit|constexpr|consteval|friend|extern|const|mutable|\[\[nodiscard\]\])\s+)*"
CALLING = r"(?:\s+(?:__fastcall|__stdcall|__cdecl|__thiscall|__vectorcall|WINAPI|CALLBACK|APIENTRY))?"
IDENT = r"[A-Za-z_]\w*"


# ---------------------------------------------------------------------------------------------------------------------------------
# Files
# ---------------------------------------------------------------------------------------------------------------------------------

def ListSources():
    out = subprocess.run(["git", "ls-files", "--cached", "--others", "--exclude-standard", "--", *SOURCE_ROOTS],
                         cwd=ROOT, capture_output=True, text=True, check=True).stdout
    # A file moved or deleted since the last commit is still in the index: only the ones on disk count.
    return sorted(p for p in out.splitlines() if p.endswith((".ixx", ".cpp")) and os.path.isfile(os.path.join(ROOT, p)))


class Source:
    """One C++ file: raw lines, lines without comments and strings, comments and string literals per line."""

    def __init__(self, path):
        self.Path = path
        with open(os.path.join(ROOT, path), "rb") as handle:
            self.Raw = handle.read()
        text = self.Raw.decode("utf-8", errors="replace")
        if text.startswith("\ufeff"):
            text = text[1:]
        self.Lines = text.replace("\r\n", "\n").split("\n")
        head = "\n".join(self.Lines[:15]).lower()
        self.IsGenerated = any(marker in head for marker in GENERATED_MARKERS)
        self.IsCpp = path.endswith(".cpp")
        self.Code, self.Comments, self.Strings, self.BlockComments = Strip(self.Lines)
        self.Module = None
        self.IsPartition = False
        for line in self.Code:
            match = re.match(r"\s*(?:export\s+)?module\s+([\w.]+)(:\w+)?\s*;", line)
            if match:
                self.Module = match.group(1)
                self.IsPartition = bool(match.group(2))
                break
        self.IsTypeModule = bool(self.Module) and "Type" in self.Module.split(".")


def Strip(lines):
    """Splits each line into code (comments removed, string contents blanked), its '//' comment and its string literals."""
    code, comments, strings, blocks = [], [], [], set()
    inBlock = False
    rawClose = None
    for index, line in enumerate(lines):
        out, comment, literals = [], "", []
        i = 0
        while i < len(line):
            if rawClose:
                end = line.find(rawClose, i)
                if end < 0:
                    i = len(line)
                    break
                i = end + len(rawClose)
                rawClose = None
                out.append('""')
                continue
            if inBlock:
                end = line.find("*/", i)
                if end < 0:
                    i = len(line)
                    break
                inBlock = False
                i = end + 2
                continue
            ch = line[i]
            if line.startswith("//", i):
                comment = line[i + 2:]
                break
            if line.startswith("/*", i):
                blocks.add(index)
                inBlock = True
                i += 2
                continue
            if ch == 'R' and line.startswith('R"', i):
                match = re.match(r'R"([^(]*)\(', line[i:])
                if match:
                    close = ")" + match.group(1) + '"'
                    end = line.find(close, i)
                    if end < 0:
                        rawClose = close
                        literals.append(line[i:])
                        i = len(line)
                        continue
                    end = end + len(close)
                    literals.append(line[i:end])
                    out.append('""')
                    i = end
                    continue
            if ch == '"' or (ch == "'" and not (i > 0 and line[i - 1].isalnum())):
                j = i + 1
                while j < len(line) and line[j] != ch:
                    j += 2 if line[j] == "\\" else 1
                if ch == '"':
                    literals.append(line[i + 1:j])
                out.append(ch + ch)
                i = j + 1
                continue
            out.append(ch)
            i += 1
        if inBlock:
            blocks.add(index)
        code.append("".join(out))
        comments.append(comment)
        strings.append(literals)
    return code, comments, strings, blocks


# ---------------------------------------------------------------------------------------------------------------------------------
# Scopes: walks the braces and hands every declaration statement to the checks with the kind of scope it lives in
# ---------------------------------------------------------------------------------------------------------------------------------

class Scope:
    def __init__(self, kind, name="", line=0):
        self.Kind = kind            # file, namespace, anonymous, class, struct, enum, block, init
        self.Name = name
        self.Line = line
        self.Access = None          # current access specifier in a class or struct
        self.AccessCount = 0        # access specifiers seen so far
        self.Statement = []         # pieces of the current statement
        self.StatementLine = None


def Classify(header):
    text = " ".join(header.split())
    if re.match(r"^(export\s+)?namespace\s*$", text):
        return "anonymous", ""
    match = re.match(r"^(?:export\s+)?namespace\s+([\w:]+)\s*$", text)
    if match:
        return "namespace", match.group(1)
    match = re.match(r"^(?:export\s+)?(?:template\s*<.*>\s*)?(class|struct|union)\s+(?:\[\[.*\]\]\s*)?(" + IDENT + r")\b[^()]*$", text)
    if match and not re.search(r"[=;]", text):
        return ("struct" if match.group(1) != "class" else "class"), match.group(2)
    match = re.match(r"^(?:export\s+)?enum\s+(?:class\s+|struct\s+)?(" + IDENT + r")", text)
    if match:
        return "enum", match.group(1)
    if re.match(r"^extern\s+\"\"", text):
        return "namespace", "extern"
    if text == "" or text.endswith(("=", ",", "(", "return", "{")) or re.search(r"[\w>\]]\s*$", text) and not re.search(r"\)\s*(const|noexcept|override|final|mutable)?\s*(->.*)?$", text):
        # A braced initializer: m_X{ 0 }, Type{ .A = 1 }, = { ... }, return { ... }
        if not re.search(r"\b(if|else|for|while|do|switch|try|catch)\b", text) and not text.endswith(")"):
            return "init", ""
    return "block", ""


def Walk(source, onStatement, onBlockHeader):
    """Calls onStatement(scope, text, line) for every statement directly inside a file, namespace, class or struct scope, and
    onBlockHeader(scope, header, line) for every function body or control block that opens there."""
    stack = [Scope("file")]
    header, headerLine = [], None
    for index, line in enumerate(source.Code):
        i = 0
        while i < len(line):
            ch = line[i]
            top = stack[-1]
            declarative = top.Kind in ("file", "namespace", "anonymous", "class", "struct")
            if ch == "{":
                text = "".join(header).strip()
                kind, name = Classify(text)
                if top.Kind in ("block", "init") and kind == "init":
                    kind = "init"
                if kind == "block" and declarative:
                    onBlockHeader(top, text, headerLine if headerLine is not None else index)
                if kind == "init" and declarative:
                    top.Statement.append(text + "{")
                    if top.StatementLine is None:
                        top.StatementLine = headerLine if headerLine is not None else index
                stack.append(Scope(kind, name, headerLine if headerLine is not None else index))
                header, headerLine = [], None
                i += 1
                continue
            if ch == "}":
                closed = stack.pop() if len(stack) > 1 else stack[0]
                parent = stack[-1]
                if closed.Kind == "init" and parent.Kind in ("file", "namespace", "anonymous", "class", "struct", "init"):
                    if parent.Kind == "init":
                        pass
                    else:
                        parent.Statement.append("}")
                        header = parent.Statement[:]
                        parent.Statement = []
                        headerLine = parent.StatementLine
                        parent.StatementLine = None
                        i += 1
                        continue
                header, headerLine = [], None
                i += 1
                continue
            if ch == ";" and declarative:
                text = ("".join(header)).strip()
                if text:
                    onStatement(top, " ".join(text.split()), headerLine if headerLine is not None else index)
                header, headerLine = [], None
                i += 1
                continue
            if declarative and top.Kind in ("class", "struct"):
                rest = line[i:]
                match = re.match(r"(public|private|protected)\s*:(?!:)", rest)
                if match and not "".join(header).strip():
                    top.Access = match.group(1)
                    top.AccessCount += 1
                    i += len(match.group(0))
                    continue
            if top.Kind in ("block", "init"):
                i += 1
                continue
            if not ch.isspace() and headerLine is None:
                headerLine = index
            header.append(ch)
            i += 1
        if stack[-1].Kind not in ("block", "init") and header and not header[-1].isspace():
            header.append(" ")
    return


# ---------------------------------------------------------------------------------------------------------------------------------
# Checks
# ---------------------------------------------------------------------------------------------------------------------------------

def LastIdentifier(text):
    text = re.sub(r"<[^<>]*>", "", re.sub(r"<[^<>]*>", "", text))
    match = re.search(r"(" + IDENT + r")\s*$", text)
    return match.group(1) if match else ""


def DropTemplate(text):
    """Removes a leading 'template <...>' and 'requires ...' clause."""
    text = text.strip()
    while text.startswith("template"):
        depth, i = 0, len("template")
        while i < len(text):
            if text[i] == "<":
                depth += 1
            elif text[i] == ">":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        text = text[i + 1:].strip()
        if text.startswith("requires"):
            match = re.match(r"requires\s+[\w:<>,\s]+?\s+(?=(?:auto|static|inline|constexpr|struct|class)\b)", text)
            text = text[match.end():] if match else text
    return text


def SplitDeclaration(statement):
    """Returns (before, name, after) where name is the declared identifier of a variable or function declaration, or None."""
    statement = DropTemplate(statement)
    depth = 0
    for i, ch in enumerate(statement):
        if ch in "<([":
            if ch == "(" and depth == 0:
                before = statement[:i]
                return before, LastIdentifier(before), statement[i:]
            depth += 1
        elif ch in ">)]":
            depth = max(0, depth - 1)
        elif depth == 0 and (ch in "{=" or (ch == ":" and statement[i:i + 2] != "::" and statement[i - 1:i + 1] != "::")):
            before = statement[:i]
            return before, LastIdentifier(before), statement[i:]
    return statement, LastIdentifier(statement), ""


def CheckFile(source, findings):
    path = source.Path

    def Add(check, line, detail=""):
        findings.append((path, line + 1, check, detail, source.Lines[line].strip() if line < len(source.Lines) else ""))

    # F: bytes of the file
    if source.IsGenerated:
        return
    if source.Raw.endswith(b"\n"):
        Add("F4", len(source.Lines) - 1)
    lf = re.sub(rb"\r\n", b"", source.Raw).count(b"\n")
    if lf:
        Add("F3", 0, f"{lf} lines end in LF only")
    if source.IsGenerated:
        return
    for index, line in enumerate(source.Lines):
        if "\t" in line:
            Add("F1", index)
        if line != line.rstrip():
            Add("F2", index)
    for index in sorted(source.BlockComments):
        Add("C1", index)
    for index, comment in enumerate(source.Comments):
        if ";" in comment and not re.search(r"(https?://|`)", comment):
            Add("C2", index)
    # S3: an opening brace that ends a line with code before it (lambdas passed as arguments and one-line guards are allowed)
    for index, line in enumerate(source.Code):
        stripped = line.rstrip()
        if stripped.endswith("{") and stripped.strip() != "{":
            before = stripped[:-1].rstrip()
            if re.search(r"(\]\s*(<[^>]*>\s*)?(\([^)]*\))?\s*(mutable\s*)?(->\s*[\w:<>]+\s*)?$)|[(,=]\s*$|\breturn\s*$", before):
                continue    # lambda or braced initializer split over lines
            if re.search(r"\)\s*(const\s*)?(noexcept\s*)?(override\s*)?(->\s*.+)?$|^\s*(else|do|try)\s*$|\b(namespace|class|struct|enum)\b", before):
                Add("S3", index)
    # L1: log texts
    for index, literals in enumerate(source.Strings):
        for literal in literals:
            if re.match(r"\[\w+\]", literal) and not re.fullmatch(r"\[\w+\]", literal) and not re.match(r"\[\w+\] (ERROR|WARNING|INFO):( |$)", literal):
                Add("L1", index, literal[:60])
    # M: module line and imports
    imports = []
    for index, line in enumerate(source.Code):
        match = re.match(r"\s*(?:export\s+)?import\s+([\w.:]+)\s*;", line)
        if match:
            imports.append((index, match.group(1)))
    if source.IsCpp and source.Module:
        for index, name in imports:
            if name.startswith(source.Module + ":"):
                Add("M1", index)
    named = [(i, n) for i, n in imports if not n.startswith(":")]
    if named and any(n == "std" for _, n in named) and named[-1][1] != "std":
        Add("M2", [i for i, n in named if n == "std"][0])
    previous = None
    for index, name in named:
        if name == "std" or name.split(".")[0] not in LAYER_RANK:
            continue
        parts = name.split(".")
        key = (LAYER_RANK[parts[0]], parts[1] if len(parts) > 1 else "", SPECIALIZATION_ORDER.get(parts[2] if len(parts) > 2 else "", 9))
        if previous and key[0] < previous[0]:
            Add("M3", index, f"{name} after a higher layer")
        elif previous and key[0] == previous[0] and key[1] == previous[1] and key[2] < previous[2]:
            Add("M3", index, f"{name}: Type, State, System, Hook order")
        previous = key
    # U4: constants with a namespace chain of two or more levels (Class::k_X is allowed)
    for index, line in enumerate(source.Code):
        if re.match(r"\s*using\b", line):
            continue
        for match in re.finditer(r"\b(?:" + IDENT + r"::){2,}k_\w+", line):
            Add("U4", index, match.group(0))
    # H1: definitions of hook statics in the .cpp
    if source.IsCpp:
        for index, line in enumerate(source.Code):
            if re.match(r"\s*[\w:<>*&\s]+\b\w+::s_\w+\s*(\{|=)", line) and not re.match(r"\s*(return|if|auto\s+\w+\s*=)", line):
                Add("H1", index)

    initialized = set()
    pair = [source.Lines]
    other = source.Path[:-4] + (".ixx" if source.IsCpp else ".cpp")
    if os.path.exists(os.path.join(ROOT, other)):
        with open(os.path.join(ROOT, other), "rb") as handle:
            pair.append(handle.read().decode("utf-8", errors="replace").split("\n"))
    for lines in pair:
        for line in lines:
            for match in re.finditer(r"(?:^\s*:|^\s*,|,)\s*(m_\w+)\s*[({]", line):
                initialized.add(match.group(1))
            # Mem-initializer with the comma at the end of the line, or split over several lines: "    m_Name(args,".
            match = re.match(r"\s*(m_\w+)\s*[({]", line)
            if match and ";" not in line:
                initialized.add(match.group(1))

    def OnStatement(scope, text, line):
        text = DropTemplate(text)
        if text.startswith(("static_assert", "friend ", "return", "typedef")) or text in ("", "}"):
            return
        if re.match(r"^(export\s+)?(module|import)\b", text):
            return
        if text.startswith("using "):
            CheckUsing(scope, text, line)
            return
        if re.match(r"^(export\s+)?(enum|struct|class|union)\b", text):
            return      # forward declaration or a definition already handled by its scope
        if scope.Kind in ("class", "struct"):
            CheckMember(scope, text, line)
        elif scope.Kind in ("namespace", "anonymous", "file"):
            CheckNamespaceStatement(scope, text, line)

    def CheckUsing(scope, text, line):
        match = re.match(r"^using\s+(" + IDENT + r")\s*=\s*(.+)$", text)
        if scope.Kind == "class" and scope.AccessCount == 1 and scope.Access == "private" or scope.Kind == "class" and scope.AccessCount == 0:
            if not match:
                Add("U1", line, "use 'using X = A::B::X;' in the first private scope of a class")
        if match:
            name, target = match.group(1), match.group(2)
            if "(" in target:
                pass        # function type: a new type of this namespace, not a renamed one
            elif LastIdentifier(re.sub(r"<.*>", "", target)) != name:
                Add("U2", line, f"{name} = {target}")
            elif scope.Kind == "anonymous":
                Add("U3", line)
            elif scope.Kind == "namespace":
                Add("U5", line, name)
        elif scope.Kind == "namespace" and not text.startswith("using namespace"):
            Add("U5", line, text[6:].rstrip(";"))
        elif scope.Kind == "class" and not text.startswith("using namespace"):
            if not (scope.AccessCount == 1 and scope.Access == "private"):
                Add("U1", line, "'using A::B::X;' is ill-formed in a class")

    def CheckMember(scope, text, line):
        if scope.Kind == "class" and scope.AccessCount == 1 and scope.Access == "private":
            Add("U1", line)
            return
        if "operator" in text:
            if not re.match(SPECIFIERS + r"auto\s+operator", text) and "operator=" in text.replace(" ", ""):
                Add("S1", line)
            return
        before, name, after = SplitDeclaration(text)
        if not name:
            return
        isFunction = after.startswith("(")
        if isFunction:
            if name in (scope.Name, "~" + scope.Name) or before.rstrip().endswith("~"):
                return
            if not re.match(SPECIFIERS + r"auto\b", before.strip()):
                Add("S1", line, name)
            if re.match(r"^(Get|Should|Must)[A-Z]", name) and (re.search(r"->\s*bool\b", after) or re.search(r"\bbool\s+\w+$", before.strip())):
                Add("N5", line, name)
            return
        isStatic = re.search(r"\bstatic\b", before)
        isConstant = re.search(r"\b(constexpr|consteval)\b", before) or re.search(r"\bconst\b", before) and isStatic
        typePart = before[:before.rfind(name)] if name in before else before
        isReference = typePart.rstrip().endswith("&")
        isBool = re.search(r"\bbool\s*$", typePart.strip()) is not None
        if isConstant:
            if not re.match(r"^k_[A-Z]", name):
                Add("N2", line, name)
        elif isStatic:
            if not re.match(r"^s_[A-Z]", name):
                Add("N3", line, name)
        elif scope.Kind == "class":
            if not re.match(r"^m_[A-Z]", name):
                Add("N1", line, name)
            elif isBool and not re.match(r"^m_Is[A-Z]", name):
                Add("N4", line, name)
        if not isReference and not after.startswith(("{", "=")) and not isConstant and name not in initialized:
            Add("S2", line, name)

    def CheckNamespaceStatement(scope, text, line):
        before, name, after = SplitDeclaration(text)
        if text.startswith("extern"):
            return
        if not name or after.startswith("("):
            if name and after.startswith("(") and not re.match(SPECIFIERS + r"auto\b", before.strip()) and not before.strip().startswith(("return", "co_")):
                if re.match(r"^" + SPECIFIERS + r"[\w:<>,*&\s]+\s+[\w:~]+$", before.strip()):
                    Add("S1", line, name)
            return
        if re.search(r"\b(constexpr|const)\b", before) and not re.match(r"^k_[A-Z]", name) and scope.Kind != "file":
            Add("N2", line, name)

    def OnBlockHeader(scope, header, line):
        # Function definitions at namespace scope (.cpp) or inline in a class
        text = DropTemplate(" ".join(header.split()))
        if not text or re.match(r"^(if|for|while|switch|else|do|try|catch|return)\b", text) or "operator" in text and "auto" in text:
            return
        match = re.match(r"^(.*?)\b((?:" + IDENT + r"::)*~?" + IDENT + r")\s*\(", text)
        if not match:
            return
        before, qualified = match.group(1).strip(), match.group(2)
        parts = qualified.split("::")
        if len(parts) >= 2 and parts[-1].lstrip("~") == parts[-2]:
            return      # constructor or destructor
        if scope.Kind in ("class", "struct") and parts[-1].lstrip("~") == scope.Name:
            return
        if before and not re.match(SPECIFIERS + r"auto" + CALLING + r"$", before) and not before.endswith(("=", "(", ",")):
            if re.match(r"^" + SPECIFIERS + r"[\w:<>,*&\s]+$", before):
                Add("S1", line, qualified)
        if scope.Kind == "class" and scope.AccessCount == 1 and scope.Access == "private":
            Add("U1", line)

    def OnScopeOpened():
        pass

    # T1: struct/enum definitions outside Type modules, or inside a class
    depthKinds = []

    def OnStatementWrapper(scope, text, line):
        OnStatement(scope, text, line)

    Walk(source, OnStatementWrapper, OnBlockHeader)
    CheckTypeDefinitions(source, Add)
    CheckThis(source, Add)
    CheckMore(source, Add)


def CheckTypeDefinitions(source, Add):
    """T1: a struct or enum body in an .ixx that is not a Type module (outside the anonymous namespace), or nested in a class."""
    stack = []
    header = []
    for index, line in enumerate(source.Code):
        for ch in line:
            if ch == "{":
                kind, name = Classify("".join(header).strip())
                if kind in ("struct", "enum"):
                    inClass = any(k == "class" for k, _ in stack)
                    inAnonymous = any(k == "anonymous" for k, _ in stack)
                    if inClass or (not source.IsCpp and not source.IsTypeModule and not inAnonymous and "/_Reflect/" not in source.Path):
                        Add("T1", index, name)
                stack.append((kind, name))
                header = []
            elif ch == "}":
                if stack:
                    stack.pop()
                header = []
            elif ch == ";":
                header = []
            else:
                header.append(ch)
        header.append(" ")


def CheckThis(source, Add):
    """S4: inside the methods of a class, a call to another method of that class without 'this->'."""
    if not source.IsCpp:
        return
    methods = collections.defaultdict(set)
    interface = source.Path[:-4] + ".ixx"
    texts = [source]
    if os.path.exists(os.path.join(ROOT, interface)):
        texts.append(Source(interface))
    for text in texts:
        current = None
        for line in text.Code:
            match = re.match(r"\s*(?:export\s+)?(?:class|struct)\s+(" + IDENT + r")\b(?!.*;)", line)
            if match:
                current = match.group(1)
            if current:
                for found in re.finditer(r"\bauto\s+(?:__\w+\s+)?(" + IDENT + r")\s*\(", line):
                    methods[current].add(found.group(1))
    if not methods:
        return
    owner, depth, base = None, 0, 0
    for index, line in enumerate(source.Code):
        match = None if owner else re.match(r"\s*(?:[\w:<>*&]+\s+)*?(" + IDENT + r")::~?" + IDENT + r"\s*\(", line)
        if match and match.group(1) in methods:
            owner, base = match.group(1), depth
        elif owner and depth > base:
            for call in re.finditer(r"(?<![\w.>:])(" + IDENT + r")\s*\(", line):
                if call.group(1) in methods[owner]:
                    Add("S4", index, call.group(1))
        opened = line.count("{")
        depth += opened - line.count("}")
        if owner and depth <= base and (opened or "}" in line) and not match:
            owner = None


# ---------------------------------------------------------------------------------------------------------------------------------
# More file checks: math, naming, comments, hooks, modules, specializations, layout, offsets
# ---------------------------------------------------------------------------------------------------------------------------------

SPANISH = re.compile(r"\b(que|para|del|los|las|una|esto|cuando|porque|pero|segun|tambien|este|esta|hay|sin|con)\b", re.I)
ACCESS_ORDERS = {(), ("private",), ("public",), ("private", "public"), ("public", "private"), ("private", "public", "private")}


def CheckMore(source, Add):
    parts = source.Module.split(".") if source.Module else []
    spec = parts[2] if len(parts) >= 3 else None
    isLayer = bool(parts) and parts[-1] == "Layer"
    isType, isHook = spec == "Type", spec == "Hook"
    declIndex, decl = None, None
    for index, line in enumerate(source.Code):
        decl = re.match(r"\s*(export\s+)?module\s+([\w.]+)(?::([\w.]+))?\s*;", line)
        if decl:
            declIndex = index
            break
    partition = decl.group(3) if decl else None

    # M4: the third level of a module is its specialization
    if len(parts) >= 3 and spec not in ("Type", "State", "System", "Hook", "Reflect") and not isLayer:
        Add("M4", declIndex or 0, source.Module)

    # M5: namespaces map 1:1 to the module (+1 level for a Type partition, the Layer uses the layer name). Reflect modules specialize
    # templates of Common::Reflect, which must be done in their namespace.
    if parts and spec != "Reflect":
        expected = "::".join(parts[:-1] if isLayer else parts)
        allowed = {expected}
        if isType and partition:
            allowed |= {expected + "::" + partition.split(".")[0], expected + "::" + partition.replace(".", "::")}
        depth = 0
        for index, line in enumerate(source.Code):
            if depth == 0:
                match = re.match(r"\s*(?:export\s+)?namespace\s+([\w:]+)\s*(?:\{.*)?$", line)
                if match and match.group(1) not in allowed:
                    Add("M5", index, f"{match.group(1)}, module {source.Module}")
            depth += line.count("{") - line.count("}")

    # A3: Type imports only Type, State imports only Type and State
    allowedSpecs = {"Type": {"Type"}, "State": {"Type", "State"}}.get(spec)
    for index, line in enumerate(source.Code):
        match = re.match(r"\s*(?:export\s+)?import\s+([\w.]+)(?::[\w.]+)?\s*;", line)
        if allowedSpecs and match:
            target = match.group(1).split(".")
            if len(target) >= 3 and target[2] in ("State", "System", "Hook") and target[2] not in allowedSpecs:
                Add("A3", index, f"{spec} imports {match.group(1)}")

    # W1, N6, A2, H2, O1: code lines
    offsets = not isType and (spec in ("System", "Hook") or source.IsCpp)
    for index, line in enumerate(source.Code):
        if re.search(r"(?<![(\w:])std::(?:max|min)\s*\(", line):
            Add("W1", index)
        if (re.search(r"[(,]\s*(?:const\s+)?[\w:<>]+(?:\s*[*&]+\s*|\s+)(?:p|pp|lp|sz|dw)[A-Z]\w*\s*(?=[,)=\[])", line)
                or re.search(r"\bauto\s*[*&]*\s*(?:p|pp|lp)[A-Z]\w*\s*[={]", line)):
            Add("N6", index)
        if not isHook and re.search(r"\bstatic\b[^;(]*\bs_Instance\b", line):
            Add("A2", index)
        match = re.match(r"\s*using\s+(\w+)\s*=\s*[^;]*\(\s*(?:__\w+\s*)?\*\s*\)", line)
        if match and (not isType or not match.group(1).endswith("Function")):
            Add("H2", index, match.group(1))
        if re.match(r"\s*typedef\b[^;]*\(\s*(?:__\w+\s*)?\*", line):
            Add("H2", index, "typedef")
        # A named constant of Artemis itself (a fake handle, a marker, an own size) may be hex. Engine values may not: their names end
        # in Offset, Rva, Bit, Mask or Opcode, and they live in Type modules (D126).
        ownConstant = re.match(r"\s*(?:static\s+)?(?:inline\s+)?constexpr\b[^;(]*\bk_(\w+)\s*\{", line)
        if ownConstant and re.search(r"(Offsets?|Rva|Bits?|Mask|Opcode)$", ownConstant.group(1)):
            ownConstant = None
        if offsets and not ownConstant and not re.search(r"IM_COL32|ImColor|Color", line):
            for match in re.finditer(r"\b0x([0-9A-Fa-f]{2,})\b", line):
                if not re.fullmatch(r"[Ff0]+", match.group(1)):
                    Add("O1", index, "0x" + match.group(1))
                    break

    # H3: the aggregating .ixx of a Hook module lives in the folder of its specialization, not in Hook/
    if (decl and decl.group(1) and not partition and isHook and "/Hook/" in source.Path
            and any(re.match(r"\s*export\s+import\s+:", line) for line in source.Code)):
        Add("H3", declIndex)

    # H4: hooked functions count the calls in flight
    if source.IsCpp and isHook:
        text = "\n".join(source.Code)
        match = re.search(r"::(Hooked\w+)\s*\(", text)
        if match and not re.search(r"InFlightScope|HookScope", text):
            Add("H4", text[:match.start()].count("\n"), match.group(1))

    # C3, C6, C7: comments
    for index, comment in enumerate(source.Comments):
        if re.search(r"\bIgnore[/\\]|\b(?:Referencia|Historia|Temporal|Tareas)[/\\]", comment):
            Add("C3", index)
        if re.match(r"\s*This\s+(?:function|method|class|struct|file)\b", comment):
            Add("C6", index)
        if re.search("[áéíóúñÁÉÍÓÚÑ¿¡]", comment) or len(SPANISH.findall(comment)) >= 2:
            Add("C7", index)

    # Y1: Layer members grouped State, System, Hook
    if isLayer:
        order, last = {"State": 0, "System": 1, "Hook": 2}, -1
        for index, comment in enumerate(source.Comments):
            match = re.match(r"\s*---\s*(\w+)\s*---", comment)
            if match:
                rank = order.get(match.group(1))
                if rank is None or rank < last:
                    Add("Y1", index, match.group(1))
                else:
                    last = rank

    # Y2: file layout around 'module;', the module line and the imports
    code, blank = source.Code, lambda i: 0 <= i < len(source.Code) and not source.Lines[i].strip()
    hasInclude = any(line.lstrip().startswith("#include") for line in source.Lines)
    if code and code[0].strip() == "module;" and not hasInclude:
        Add("Y2", 0, "'module;' without includes")
    if declIndex:
        if hasInclude and not blank(declIndex - 1):
            Add("Y2", declIndex, "blank line before the module line")
        if not source.IsCpp and not blank(declIndex + 1):
            Add("Y2", declIndex, "blank line after the module line")
    imports = [i for i, line in enumerate(code) if re.match(r"\s*(?:export\s+)?import\s", line)]
    if imports:
        for i in imports:
            # std goes right after the other imports. After partition imports only, it is the first other import: blank line before.
            previous = [j for j in imports if j < i]
            afterPartitions = previous and re.match(r"\s*(?:export\s+)?import\s+:", code[previous[-1]])
            if re.match(r"\s*import\s+std\s*;", code[i]) and len(imports) > 1 and blank(i - 1) and not afterPartitions:
                Add("Y2", i, "no blank line before 'import std;'")
        if not blank(imports[-1] + 1) and imports[-1] + 1 < len(code):
            Add("Y2", imports[-1], "blank line after the imports")
        if not source.IsCpp:
            partitions = [i for i in imports if re.match(r"\s*(?:export\s+)?import\s+:", code[i])]
            named = [i for i in imports if i not in partitions]
            if partitions and named and named[0] > partitions[-1] and not blank(partitions[-1] + 1):
                Add("Y2", partitions[-1], "blank line between partition imports and other imports")

    # Y3: sections of a class in an .ixx: [private: usings] public: [private:]
    if not source.IsCpp:
        depth, pending, classes = 0, None, []
        for index, line in enumerate(code):
            match = re.match(r"\s*(?:export\s+)?(?:template\s*<.*>\s*)?class\s+(\w+)(?!.*;\s*$)", line)
            if match:
                pending = (match.group(1), index)
            label = re.match(r"\s*(public|private|protected)\s*:(?!:)", line)
            if label and classes and depth == classes[-1][0] + 1:
                classes[-1][2].append(label.group(1))
            for ch in line:
                if ch == "{":
                    if pending:
                        classes.append((depth, pending[0], [], pending[1]))
                        pending = None
                    depth += 1
                elif ch == "}":
                    depth -= 1
                    if classes and depth == classes[-1][0]:
                        _, name, labels, at = classes.pop()
                        if tuple(labels) not in ACCESS_ORDERS:
                            Add("Y3", at, f"{name}: {', '.join(labels)}")
                elif ch == ";" and pending and pending[1] == index:
                    pending = None


def CheckProjectMore(sources, findings):
    # P1: every source is in the Visual Studio projects
    def Read(path):
        full = os.path.join(ROOT, path)
        return open(full, encoding="utf-8-sig", errors="replace").read() if os.path.exists(full) else None
    pyproj = Read("Artemis-MCC.pyproj")
    projects = {}
    for source in sources:
        root = next((r for r in PROJECT_OF if source.Path.startswith(r)), None)
        if root is None:
            continue
        project = PROJECT_OF[root]
        if project not in projects:
            projects[project] = (Read(project + ".vcxproj"), Read(project + ".vcxproj.filters"))
        # A project lists its files relative to its own folder.
        name = source.Path[len(os.path.dirname(project)) + 1:].replace("/", "\\")
        for text, label in zip(projects[project], (os.path.basename(project) + ".vcxproj", os.path.basename(project) + ".vcxproj.filters")):
            if text is not None and f'"{name}"' not in text:
                findings.append((source.Path, 1, "P1", f"missing in {label}", ""))
    if pyproj is not None:
        out = subprocess.run(["git", "ls-files", "--cached", "--others", "--exclude-standard", "--", "Tools/*.py", "Tools/**/*.py"],
                             cwd=ROOT, capture_output=True, text=True).stdout
        for path in sorted(set(out.splitlines())):
            if f'"{path.replace("/", chr(92))}"' not in pyproj:
                findings.append((path, 1, "P1", "missing in Artemis-MCC.pyproj", ""))


def DeadMethods(sources):
    """X1 (only with --dead): methods declared in an .ixx whose name appears at most twice in the code (declaration and definition)."""
    words = collections.Counter()
    for source in sources:
        for line in source.Code:
            words.update(re.findall(r"\b[A-Za-z_]\w*\b", line))
    findings = []
    for source in sources:
        if source.IsCpp or source.IsGenerated:
            continue
        for index, line in enumerate(source.Code):
            match = re.match(r"\s*(?:(?:static|virtual|inline|constexpr|\[\[nodiscard\]\])\s+)*auto\s+([A-Z]\w*)\s*\(", line)
            if match and "override" not in line and not match.group(1).startswith("Hooked") and words[match.group(1)] <= 2:
                findings.append((source.Path, index + 1, "X1", match.group(1), line.strip()))
    return findings


# ---------------------------------------------------------------------------------------------------------------------------------
# Project checks
# ---------------------------------------------------------------------------------------------------------------------------------

def CheckProject(sources, findings):
    # A1: imports must not go up the layers
    for source in sources:
        if not source.Module:
            continue
        own = source.Module.split(".")[0]
        if own not in LAYER_RANK or LAYER_RANK[own] >= 30:
            continue
        for index, line in enumerate(source.Code):
            match = re.match(r"\s*(?:export\s+)?import\s+([\w.]+)\s*;", line)
            if not match:
                continue
            target = match.group(1).split(".")[0]
            if target in LIBRARY_MODULES:
                continue
            if target in LAYER_RANK and LAYER_RANK[target] > LAYER_RANK[own] and LAYER_RANK[target] < 30:
                findings.append((source.Path, index + 1, "A1", f"{own} imports {match.group(1)}", line.strip()))
    # D1, D2: constants defined in several files
    definitions = collections.defaultdict(list)
    byValue = collections.defaultdict(list)
    for source in sources:
        if source.IsGenerated:
            continue
        for index, line in enumerate(source.Code):
            match = re.search(r"\b(k_\w+)\s*\{\s*([^{}]+?)\s*\}\s*;", line)
            if not match or not re.search(r"\b(constexpr|const)\b", line):
                continue
            name, value = match.group(1), " ".join(match.group(2).split())
            definitions[(name, value)].append((source.Path, index))
            if re.fullmatch(r"0x[0-9A-Fa-f]{3,}", value):
                byValue[value.upper()].append((name, source.Path, index))
    for (name, value), places in definitions.items():
        files = {p for p, _ in places}
        if len(files) > 1:
            for path, index in places:
                findings.append((path, index + 1, "D1", f"{name} {{ {value} }} in {len(files)} files", ""))
    for value, places in byValue.items():
        names = {n for n, _, _ in places}
        files = {p for _, p, _ in places}
        if len(names) > 1 and len(files) > 1:
            for name, path, index in places:
                findings.append((path, index + 1, "D2", f"{value} as {', '.join(sorted(names))}", ""))
    CheckProjectMore(sources, findings)


# ---------------------------------------------------------------------------------------------------------------------------------
# Baseline and output
# ---------------------------------------------------------------------------------------------------------------------------------

def Key(finding):
    path, _, check, detail, text = finding
    return f"{path}\t{check}\t{' '.join((text or detail).split())}"


def LoadBaseline():
    counts = collections.Counter()
    if os.path.exists(BASELINE):
        with open(BASELINE, encoding="utf-8") as handle:
            for line in handle:
                line = line.rstrip("\n")
                if line and not line.startswith("#"):
                    counts[line] += 1
    return counts


def SaveBaseline(findings):
    with open(BASELINE, "w", encoding="utf-8", newline="\r\n") as handle:
        handle.write("# Findings accepted when each check was added. Regenerate with: python Tools/check_rules.py --update-baseline\n")
        for key in sorted(Key(f) for f in findings):
            handle.write(key + "\n")


# Checks that the files in Tools/check_rules_cases/ must trigger (one deliberate violation each).
SELF_TEST_EXPECTED = {"F1", "F2", "S1", "S2", "S3", "S4", "N1", "N2", "N3", "N4", "N5", "U1", "U2", "U3", "U4", "U5", "T1", "M1", "M2", "M3",
                      "C1", "C2", "L1", "A2", "A3", "M4", "M5", "W1", "N6", "C3", "C6", "C7", "H2", "H3", "H4", "O1", "Y1", "Y2",
                      "Y3"}


def SelfTest():
    findings = []
    cases = os.path.join(ROOT, "Tools", "check_rules_cases")
    for folder, _, names in os.walk(cases):
        for name in sorted(names):
            if name.endswith((".ixx", ".cpp")):
                CheckFile(Source(os.path.relpath(os.path.join(folder, name), ROOT).replace(os.sep, "/")), findings)
    found = {f[2] for f in findings}
    missing = sorted(SELF_TEST_EXPECTED - found)
    for path, line, check, detail, _ in findings:
        print(f"{path}:{line}: {check} {detail}")
    print("Self test: " + ("OK, every check fired." if not missing else "FAILED, these checks did not fire: " + ", ".join(missing)))
    return 1 if missing else 0


def Main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--summary", action="store_true")
    parser.add_argument("--update-baseline", action="store_true")
    parser.add_argument("--files", nargs="*")
    parser.add_argument("--only")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--dead", action="store_true")
    arguments = parser.parse_args()
    if arguments.self_test:
        return SelfTest()

    paths = ListSources()
    sources = [Source(p) for p in paths]
    if arguments.dead:
        dead = DeadMethods(sources)
        for path, line, check, detail, _ in dead:
            print(f"{path}:{line}: X1 {detail}")
        print(f"{len(dead)} possibly dead methods (check each one: planned systems are kept, rules_cpp.txt 1).")
        return 0
    findings = []
    selected = set(arguments.files or [])
    for source in sources:
        if selected and source.Path not in {s.replace("\\", "/") for s in selected}:
            continue
        CheckFile(source, findings)
    CheckProject(sources, findings)
    if arguments.only:
        wanted = set(arguments.only.split(","))
        findings = [f for f in findings if f[2] in wanted]
    findings.sort(key=lambda f: (f[0], f[1], f[2]))

    if arguments.update_baseline:
        SaveBaseline(findings)
        print(f"Baseline written: {len(findings)} findings in {BASELINE}.")
        return 0

    baseline = LoadBaseline()
    new, old = [], []
    for finding in findings:
        key = Key(finding)
        if baseline[key] > 0:
            baseline[key] -= 1
            old.append(finding)
        else:
            new.append(finding)

    if arguments.summary:
        newCounts = collections.Counter(f[2] for f in new)
        oldCounts = collections.Counter(f[2] for f in old)
        print(f"{'check':6} {'new':>6} {'baseline':>9}  rule")
        for check, text in CHECKS.items():
            if newCounts[check] or oldCounts[check]:
                print(f"{check:6} {newCounts[check]:6} {oldCounts[check]:9}  {text}")
        print(f"{'total':6} {len(new):6} {len(old):9}  ({len(sources)} files)")
        return 1 if new else 0

    shown = findings if arguments.all else new
    for path, line, check, detail, text in shown:
        suffix = f" ({detail})" if detail else ""
        print(f"{path}:{line}: {check} {CHECKS[check]}{suffix}")
    print(f"{len(new)} new, {len(old)} in the baseline, {len(sources)} files.")
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(Main())
