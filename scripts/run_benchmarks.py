#!/usr/bin/env python3
"""Run each config against each request file and optionally save a CSV table."""

import argparse
import csv
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument(
        '--binary',
        type=Path,
        default=Path('build/cache_runner')
    )
    parser.add_argument(
        '--configs',
        type=Path,
        nargs='+',
        required=True
    )
    parser.add_argument(
        '--requests',
        type=Path,
        nargs='+',
        required=True
    )
    parser.add_argument('--csv', type=Path)
    parser.add_argument(
        '--delimiter',
        choices=[';', ','],
        default=';',
        help='CSV separator (default: semicolon for regional Excel settings)'
    )

    parser.add_argument('--distribution', choices=['equal', 'linear', 'geometric'], default='geometric')
    parser.add_argument('--ratio', type=int, default=2, help='Geometric factor, at least 2')
    parser.add_argument('--sizes', help='Exact comma-separated capacities, instead of a distribution')
    args = parser.parse_args()
    if args.ratio < 2:
        parser.error('--ratio must be at least 2')
    options = []
    if args.sizes:
        options += ['--sizes', args.sizes]
    else:
        options += ['--distribution', args.distribution]
        if args.distribution == 'geometric':
            options += ['--ratio', str(args.ratio)]
    rows = []

    for request in args.requests:
        for config in args.configs:
            result = subprocess.run(
                [
                    str(args.binary.resolve()),
                    str(config),
                    str(request),
                    '--json', *options
                ],
                text=True,
                capture_output=True,
                check=True
            )

            stats = json.loads(result.stdout)

            rows.append({
                'requests_file': str(request),
                'config': str(config),
                'mode': 'independent',
                'distribution': 'explicit' if args.sizes else args.distribution,
                'ratio': args.ratio if not args.sizes and args.distribution == 'geometric' else '',
                'level_sizes': ' / '.join(str(level['capacity']) for level in stats['levels']),
                'capacity': sum(
                    level['capacity'] for level in stats['levels']
                ),
                'requests': stats['requests'],
                'hits': stats['hits'],
                'misses': stats['misses'],
                'level_hits': ' / '.join(
                    str(level['hits']) for level in stats['levels']
                )
            })

    print(
        f'{"Workload":<24} {"Config":<24} '
        f'{"Capacity":>8} {"Requests":>9} '
        f'{"Hits":>8} {"Misses":>8}  Level hits'
    )

    for row in rows:
        print(
            f'{Path(row["requests_file"]).stem:<24} '
            f'{Path(row["config"]).stem:<24} '
            f'{row["capacity"]:>8} {row["requests"]:>9} '
            f'{row["hits"]:>8} {row["misses"]:>8}  '
            f'{row["level_hits"]}'
        )

    if args.csv:
        args.csv.parent.mkdir(parents=True, exist_ok=True)

        with args.csv.open(
            'w',
            newline='',
            encoding='utf-8-sig'
        ) as output:
            writer = csv.DictWriter(
                output,
                fieldnames=rows[0].keys(),
                delimiter=args.delimiter
            )

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