#!/usr/bin/env python3
"""
Pipeline de validacion de campos (Artemis) — analiza el NDJSON que produce
Common.Reflect.Dump (Domain, lado C++) y rankea cada "path" de campo por
señal, para decidir que campos de Resolved::Stats entran al espacio de
observacion egocentrico de la IA (capa 6) y cuales se descartan o se
re-verifican in-game.

Formato de entrada esperado (una linea NDJSON por objeto/tag muestreado):
    {"tick": 118422, "tag": "weapons\\rifle\\ar", "fourcc": "weap",
     "handle": "0x8000012A", "fields": {"AutoaimAngle": "2.5",
     "PrimaryBarrel.RoundsPerSecondMin": "8.3", ...}}

Cada dict "fields" es exactamente lo que junta DumpToRows() del lado C++
(path -> valor formateado a texto); este script no sabe nada de Domain,
solo agrega texto.

Uso:
    python field_usage_report.py dump.ndjson --tag weap --out report.csv

Con el dump por-tag del Stats Inspector (boton "Dump all tags of this type",
archivo stats_dump_<fourcc>.ndjson en la carpeta AppData de Artemis; una fila
por TAG del mapa cargado, no por instancia colocada):
    python field_usage_report.py stats_dump_eqip.ndjson --tag eqip

Un path solo existe en las filas donde el campo aparece; un vector vacio emite
"<path>#count" = 0. Por eso, ademas de coverage/variance, se reporta:
  - presence:     fraccion de tags donde el path existe (bajo = campo de una
                  variante que pocos tags usan; NO es señal de campo muerto).
  - nonzero_tags: en cuantos tags vale distinto de cero. Es el dato clave:
                  0 => el campo nunca se usa en este mapa (candidato a
                  eliminar); N => se usa en N tags, ver "examples".
  - examples:     hasta --examples tags con su valor distinto de cero.

Para arrays grandes (coll: mallas con miles de triangulos) usar
    python field_usage_report.py stats_dump_coll.ndjson --tag coll --collapse-indices --minus-one-is-value
que agrupa por campo en vez de por elemento.

Metricas por path (agrupando por fourcc, ya que el mismo nombre de campo
puede repetirse entre tags):
  - coverage:  fraccion de muestras donde el valor difiere de un "cero"
               (0, 0.0, false, "", "<empty>", -1) -> cerca de 0 = probable
               mis-offset o campo no usado por el motor en este contexto.
  - variance:  fraccion de valores DISTINTOS sobre el total de muestras
               (para texto: distintos/total; para numeros: coef. de
               variacion si aplica) -> constante en todo el sample =
               sospechoso o simplemente irrelevante aca.
  - signal:    coverage * variance, orden sugerido para decidir que mirar
               primero. No reemplaza la correlacion con eventos conocidos
               (ver paper: jpt!.DamageLowerBound vs delta de vida
               observado) — esa parte es ad-hoc por campo y se hace
               aparte, este reporte solo prioriza donde mirar.
"""

import argparse
import csv
import json
import re
import statistics
import sys
from collections import defaultdict

ZERO_LIKE = {"0", "0.0", "false", "", "<empty>", "-1"}
INDEX_RE = re.compile(r"\[\d+\]")


def is_zero_like(value: str) -> bool:
    return value in ZERO_LIKE


def try_float(value: str):
    try:
        return float(value)
    except ValueError:
        return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("ndjson_path", help="Archivo NDJSON volcado por el dumper de C++")
    parser.add_argument("--tag", help="Filtrar por fourcc (bipd, hlmt, jpt!, ...)", default=None)
    parser.add_argument("--out", help="CSV de salida", default="field_usage_report.csv")
    parser.add_argument("--min-samples", type=int, default=1,
                        help="Ignorar paths con menos muestras que esto (default: 1; con 5 se "
                             "esconden los campos de variantes que usan solo 1-4 tags)")
    parser.add_argument("--collapse-indices", action="store_true",
                        help="Agrupa Meshes[3].Triangles[41].Material como Meshes[].Triangles[].Material: "
                             "un path por campo en vez de uno por elemento (imprescindible para coll, "
                             "160k paths sin esto). samples pasa a contar elementos; nonzero_tags sigue "
                             "contando tags distintos.")
    parser.add_argument("--minus-one-is-value", action="store_true",
                        help="No tratar -1 como cero (indices con -1 = 'sin padre/hermano' son datos "
                             "validos; con esto aparecen con coverage alto en vez de como campo muerto).")
    parser.add_argument("--max-list", type=int, default=60,
                        help="Maximo de lineas por lista impresa (default: 60; el CSV siempre es completo)")
    parser.add_argument("--examples", type=int, default=3,
                        help="Tags de ejemplo con valor distinto de cero por path (default: 3)")
    args = parser.parse_args()

    zero_like = ZERO_LIKE - {"-1"} if args.minus_one_is_value else ZERO_LIKE

    # path -> lista de valores (como texto) y el tag de cada valor
    samples: dict[str, list[str]] = defaultdict(list)
    tags_by_path: dict[str, list[str]] = defaultdict(list)
    fourcc_by_path: dict[str, str] = {}
    all_tags: set[str] = set()
    total_objects = 0

    with open(args.ndjson_path, "r", encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            row = json.loads(line)

            if args.tag and row.get("fourcc") != args.tag:
                continue

            total_objects += 1
            fourcc = row.get("fourcc", "?")
            tag = row.get("tag", "?")
            all_tags.add(tag)

            for path, value in row.get("fields", {}).items():
                if args.collapse_indices:
                    path = INDEX_RE.sub("[]", path)
                key = f"{fourcc}::{path}"
                samples[key].append(value)
                tags_by_path[key].append(tag)
                fourcc_by_path[key] = fourcc

    if total_objects == 0:
        print("Sin objetos que matcheen el filtro. Revisa --tag o el archivo de entrada.", file=sys.stderr)
        return 1

    rows = []
    for key, values in samples.items():
        if len(values) < args.min_samples:
            continue

        n = len(values)
        nonzero = sum(0 if value in zero_like else 1 for value in values)
        coverage = nonzero / n

        distinct = len(set(values))
        variance = distinct / n

        numeric = [try_float(value) for value in values]
        numeric = [value for value in numeric if value is not None]
        stdev = statistics.pstdev(numeric) if len(numeric) > 1 else None
        mean = statistics.fmean(numeric) if numeric else None

        signal = coverage * variance

        tags = tags_by_path[key]
        nonzero_pairs = [(t, v) for t, v in zip(tags, values) if v not in zero_like]
        examples = "; ".join(f"{t}={v}" for t, v in nonzero_pairs[:args.examples])

        rows.append({
            "fourcc": fourcc_by_path[key],
            "path": key.split("::", 1)[1],
            "samples": n,
            "tags": len(set(tags)),
            "presence": round(len(set(tags)) / max(1, len(all_tags)), 4),
            "nonzero_tags": len({t for t, _ in nonzero_pairs}),
            "coverage": round(coverage, 4),
            "variance": round(variance, 4),
            "signal": round(signal, 4),
            "mean": round(mean, 4) if mean is not None else "",
            "stdev": round(stdev, 4) if stdev is not None else "",
            "examples": examples,
        })

    rows.sort(key=lambda r: r["signal"])

    with open(args.out, "w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=[
            "fourcc", "path", "samples", "tags", "presence", "nonzero_tags", "coverage", "variance",
            "signal", "mean", "stdev", "examples"])
        writer.writeheader()
        writer.writerows(rows)

    dead_or_suspect = [r for r in rows if r["signal"] < 0.05]

    # Campos que existen pero nunca valen distinto de cero, y bloques (vectores)
    # vacios en todos los tags: candidatos directos a eliminar.
    never_used = [r for r in rows
                  if r["nonzero_tags"] == 0 and not r["path"].endswith("#count")]

    used_blocks: set[str] = set()
    counted_blocks: set[str] = set()
    for r in rows:
        if r["path"].endswith("#count"):
            counted_blocks.add(r["path"][: -len("#count")])
        elif "[" in r["path"] and r["nonzero_tags"] > 0:
            used_blocks.add(r["path"].split("[", 1)[0])
    empty_blocks = sorted(b for b in counted_blocks if b not in used_blocks)

    print(f"Objetos/tags muestreados: {total_objects}")
    print(f"Paths analizados: {len(rows)}")
    print(f"Reporte completo: {args.out}")
    print()
    print(f"Candidatos a revisar primero (signal < 0.05, posible campo muerto/mal decodificado): {len(dead_or_suspect)}")
    for row in dead_or_suspect[:args.max_list]:
        print(f"  [{row['fourcc']}] {row['path']}  "
              f"(coverage={row['coverage']}, variance={row['variance']}, n={row['samples']})")

    print()
    print(f"Bloques (vectores) vacios en TODOS los tags donde aparecen: {len(empty_blocks)}")
    for block in empty_blocks[:args.max_list]:
        print(f"  {block}")
    if len(empty_blocks) > args.max_list:
        print(f"  ... (+{len(empty_blocks) - args.max_list} mas; ver CSV)")

    print()
    print(f"Campos que nunca valen distinto de cero: {len(never_used)}")
    for row in never_used[:args.max_list]:
        print(f"  [{row['fourcc']}] {row['path']}  (muestras={row['samples']}, tags={row['tags']})")
    if len(never_used) > args.max_list:
        print(f"  ... (+{len(never_used) - args.max_list} mas; ver CSV)")
    print()
    if args.minus_one_is_value:
        print("Nota: -1 se trato como valor (--minus-one-is-value).")
    else:
        print("Nota: -1 cuenta como cero; un campo con nonzero_tags=0 puede valer -1 (o ser un indice")
        print("valido 0/-1) y ser informativo. Repite con --minus-one-is-value si son indices.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
