import re
import sys
from collections import defaultdict

def parse_log(filepath):
    scalar_values  = defaultdict(set)
    array_values   = defaultdict(lambda: defaultdict(set))

    current_section = None
    current_array   = None

    re_tag_start  = re.compile(r'=== BIPD RAW: .+ ===')
    re_tag_end    = re.compile(r'=== END BIPD:')
    re_array_hdr  = re.compile(r'\[(\w+): (\d+)\]')
    re_section    = re.compile(r'\[(\w+)\]$')
    re_array_item = re.compile(r'\[(\d+)\] (.+)')
    re_scalar     = re.compile(r'([\w.]+):\s+(.+)')

    with open(filepath, encoding='utf-8', errors='replace') as f:
        for raw_line in f:
            line = re.sub(r'^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\s+', '', raw_line).rstrip()

            if re_tag_start.search(line) or re_tag_end.search(line):
                current_section = None
                current_array   = None
                continue

            stripped = line.strip()
            if not stripped:
                continue

            m_arr = re_array_hdr.match(stripped)
            if m_arr and stripped.endswith(']'):
                current_array   = m_arr.group(1)
                current_section = None
                continue

            m_sec = re_section.match(stripped)
            if m_sec:
                current_section = m_sec.group(1)
                current_array   = None
                continue

            m_item = re_array_item.match(stripped)
            if m_item and current_array:
                pairs = m_item.group(2)
                for kv in re.finditer(r'([\w.]+):(-?[\d.]+)', pairs):
                    key, val = kv.group(1), kv.group(2)
                    try:
                        array_values[current_array][key].add(float(val))
                    except ValueError:
                        array_values[current_array][key].add(val)
                continue

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
    return all(v == 0.0 or v == '0' or v == '' for v in values)


def report(scalar_values, array_values):
    print("=" * 70)
    print("SCALAR FIELDS - non-zero in at least one tag")
    print("=" * 70)
    for field in sorted(scalar_values):
        vals = scalar_values[field]
        if not is_always_zero(vals):
            non_zero = sorted(v for v in vals if v != 0.0 and v != '0')
            print(f"  {field}")
            print(f"    non-zero values seen: {non_zero[:10]}")

    print()
    print("=" * 70)
    print("SCALAR FIELDS - always zero (candidates to DISCARD)")
    print("=" * 70)
    for field in sorted(scalar_values):
        if is_always_zero(scalar_values[field]):
            print(f"  {field}")

    print()
    print("=" * 70)
    print("ARRAY BLOCKS - field breakdown")
    print("=" * 70)
    for block in sorted(array_values):
        print(f"\n  [{block}]")
        for field in sorted(array_values[block]):
            vals = array_values[block][field]
            if is_always_zero(vals):
                print(f"    {field}: ALWAYS ZERO")
            else:
                non_zero = sorted(v for v in vals if v != 0.0 and v != '0')
                print(f"    {field}: non-zero seen -> {non_zero[:10]}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python filter_bipd_fields.py <logfile.txt>")
        sys.exit(1)

    scalar_values, array_values = parse_log(sys.argv[1])
    report(scalar_values, array_values)