#!/usr/bin/env python3
"""Run every config against every request file and optionally save a CSV table."""

import argparse
import csv
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=Path('build/cache_runner'))
    parser.add_argument('--configs', type=Path, nargs='+', required=True)
    parser.add_argument('--requests', type=Path, nargs='+', required=True)
    parser.add_argument('--csv', type=Path)
    args = parser.parse_args()
    rows = []
    for request in args.requests:
        for config in args.configs:
            result = subprocess.run(
                [str(args.binary.resolve()), str(config), str(request), '--json'],
                text=True, capture_output=True, check=True)
            stats = json.loads(result.stdout)
            rows.append({
                'requests_file': str(request), 'config': str(config),
                'capacity': sum(level['capacity'] for level in stats['levels']),
                'requests': stats['requests'], 'hits': stats['hits'],
                'misses': stats['misses'],
                'level_hits': ' / '.join(str(level['hits']) for level in stats['levels']),
            })
    print(f'{"Workload":<24} {"Config":<24} {"Capacity":>8} {"Requests":>9} '
          f'{"Hits":>8} {"Misses":>8}  Level hits')
    for row in rows:
        print(f'{Path(row["requests_file"]).stem:<24} {Path(row["config"]).stem:<24} '
              f'{row["capacity"]:>8} {row["requests"]:>9} {row["hits"]:>8} '
              f'{row["misses"]:>8}  {row["level_hits"]}')
    if args.csv:
        args.csv.parent.mkdir(parents=True, exist_ok=True)
        with args.csv.open('w', newline='', encoding='utf-8') as output:
            writer = csv.DictWriter(output, fieldnames=rows[0].keys())
            writer.writeheader()
            writer.writerows(rows)
        print(f'Saved: {args.csv}')


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(error.stderr.strip() or str(error), file=sys.stderr)
        sys.exit(1)
    except (OSError, ValueError, KeyError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
