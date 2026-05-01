import re
import sys
from collections import defaultdict

def parse_log(filepath):
    """
    Parse the PROJ RAW log and return: 
    scalar_values: dict[field_name] -> set of float/int values ​​across all tags 
    array_values: dict[block_name][field_name] -> set of values ​​across all entries
    """
    scalar_values  = defaultdict(set)
    array_values   = defaultdict(lambda: defaultdict(set))

    current_section = None
    current_array   = None

    # Patrones
    re_tag_start  = re.compile(r'=== PROJ RAW: .+ ===')
    re_section    = re.compile(r'\[(\w+)\]')
    re_array_hdr  = re.compile(r'\[(\w+): \d+\]')
    re_array_item = re.compile(r'\[(\d+)\] (.+)')
    re_scalar     = re.compile(r'(\w[\w.]+):\s+([^\s].+)')

    with open(filepath, encoding='utf-8') as f:
        for raw_line in f:
            # Remove timestamp if it exists (format: "2026-04-30 14:18:23 ")
            line = re.sub(r'^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\s+', '', raw_line).rstrip()

            if re_tag_start.search(line):
                current_section = None
                current_array   = None
                continue

            if line.strip().startswith('==='):
                continue

            stripped = line.strip()

            # Section with array (has a number: [BruteGrenade: 3])
            m_arr_hdr = re_array_hdr.match(stripped)
            if m_arr_hdr and ':' in stripped and stripped[-1] == ']':
                current_array   = m_arr_hdr.group(1)
                current_section = None
                continue

            # Scalar section ([Kinematics], [Guidance], etc.)
            m_sec = re_section.match(stripped)
            if m_sec and stripped.endswith(']') and ':' not in stripped:
                current_section = m_sec.group(1)
                current_array   = None
                continue

            # Array item: [0] Key:val Key2:val2 ...
            m_item = re_array_item.match(stripped)
            if m_item and current_array:
                pairs = m_item.group(2)
                for kv in re.finditer(r'(\w+):(-?[\d.]+)', pairs):
                    key, val = kv.group(1), kv.group(2)
                    try:
                        array_values[current_array][key].add(float(val))
                    except ValueError:
                        array_values[current_array][key].add(val)
                continue

            # Scalar normal: Key: value
            m_sc = re_scalar.match(stripped)
            if m_sc and current_section and not current_array:
                key = f"{current_section}.{m_sc.group(1)}"
                val = m_sc.group(2).strip()
                try:
                    scalar_values[key].add(float(val))
                except ValueError:
                    scalar_values[key].add(val)

    return scalar_values, array_values


def is_always_zero(values):
    """True if all values ​​are 0 or empty."""
    return all(v == 0.0 or v == '0' or v == '' for v in values)


def report(scalar_values, array_values):
    print("=" * 70)
    print("SCALAR FIELDS — non-zero in at least one tag")
    print("=" * 70)
    for field in sorted(scalar_values):
        vals = scalar_values[field]
        if not is_always_zero(vals):
            non_zero = sorted(v for v in vals if v != 0.0)
            print(f"  {field}")
            print(f"    non-zero values seen: {non_zero[:10]}")

    print()
    print("=" * 70)
    print("SCALAR FIELDS — always zero (candidates to DISCARD)")
    print("=" * 70)
    for field in sorted(scalar_values):
        if is_always_zero(scalar_values[field]):
            print(f"  {field}")

    print()
    print("=" * 70)
    print("ARRAY BLOCKS — field breakdown")
    print("=" * 70)
    for block in sorted(array_values):
        print(f"\n  [{block}]")
        for field in sorted(array_values[block]):
            vals = array_values[block][field]
            if is_always_zero(vals):
                print(f"    {field}: ALWAYS ZERO")
            else:
                non_zero = sorted(v for v in vals if v != 0.0)
                print(f"    {field}: non-zero seen -> {non_zero[:10]}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python filter_proj_fields.py <logfile.txt>")
        sys.exit(1)

    scalar_values, array_values = parse_log(sys.argv[1])
    report(scalar_values, array_values)