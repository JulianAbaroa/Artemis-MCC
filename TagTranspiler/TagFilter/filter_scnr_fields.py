import re
import sys
from collections import defaultdict, OrderedDict

def parse_log(filepath):
    # scnr tiene un solo tag por mapa, guardamos los valores directamente
    scalar_values  = OrderedDict()
    array_values   = defaultdict(lambda: defaultdict(list))

    current_section = None
    current_array   = None

    re_tag_start  = re.compile(r'=== SCNR RAW: .+ ===')
    re_tag_end    = re.compile(r'=== END SCNR:')
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
                idx   = int(m_item.group(1))
                pairs = m_item.group(2)
                entry = {'_index': idx}
                for kv in re.finditer(r'([\w.]+):(-?[\w.]+)', pairs):
                    entry[kv.group(1)] = kv.group(2)
                array_values[current_array][idx] = entry
                continue

            m_sc = re_scalar.match(stripped)
            if m_sc and current_section and not current_array:
                key = f"{current_section}.{m_sc.group(1)}"
                scalar_values[key] = m_sc.group(2).strip()

    return scalar_values, array_values


def is_zero(val):
    try:
        return float(val) == 0.0
    except (ValueError, TypeError):
        return val == '' or val is None


def report(scalar_values, array_values):
    print("=" * 70)
    print("SCNR - Single tag per map. All field values shown.")
    print("=" * 70)

    print("\n[SCALAR FIELDS]")
    for field, val in scalar_values.items():
        marker = " <-- ZERO" if is_zero(val) else ""
        print(f"  {field}: {val}{marker}")

    print()
    print("[ARRAY BLOCKS]")
    for block in sorted(array_values):
        entries = array_values[block]
        print(f"\n  [{block}: {len(entries)} entries]")
        for idx in sorted(entries):
            entry = entries[idx]
            parts = [f"{k}={v}" for k, v in entry.items() if k != '_index']
            # Flag non-zero fields
            non_zero = [p for p in parts
                        if not is_zero(p.split('=')[1] if '=' in p else '0')]
            zero     = [p for p in parts
                        if is_zero(p.split('=')[1] if '=' in p else '0')]
            print(f"    [{idx}]")
            if non_zero:
                print(f"      non-zero: {', '.join(non_zero)}")
            if zero:
                print(f"      zero:     {', '.join(zero)}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python filter_scnr_fields.py <logfile.txt>")
        sys.exit(1)

    scalar_values, array_values = parse_log(sys.argv[1])
    report(scalar_values, array_values)