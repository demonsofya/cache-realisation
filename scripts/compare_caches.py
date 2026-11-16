#!/usr/bin/env python3
#Compare the former GoogleTest scenarios without GoogleTest output.

import argparse
import csv
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def ideal_hits(data, capacity):
    """Belady reference for equal-size objects and an initially empty cache."""
    if capacity == 0:
        return 0

    never = len(data)
    next_use = [never] * len(data)
    positions = {}

    for index in range(len(data) - 1, -1, -1):
        next_use[index] = positions.get(data[index], never)
        positions[data[index]] = index

    resident = {}
    hits = 0

    for index, key in enumerate(data):
        if key in resident:
            hits += 1
        elif len(resident) == capacity:
            del resident[max(resident, key=resident.get)]

        resident[key] = next_use[index]

    return hits


def load_requests(path):
    tokens = [
        token
        for line in path.read_text().splitlines()
        for token in line.split('#', 1)[0].split()
    ]

    values = list(map(int, tokens))

    if len(values) < 2 or any(value < 0 for value in values):
        raise ValueError(
            f'{path}: expected nonnegative capacity, count and keys'
        )

    capacity, count, *data = values

    if len(data) != count:
        raise ValueError(
            f'{path}: declared request count does not match data'
        )

    return capacity, data


def run_cache(binary, config, requests, count, bound):
    result = subprocess.run(
        [str(binary), str(config), str(requests), '--json'],
        text=True,
        capture_output=True,
        check=True
    )

    stats = json.loads(result.stdout)
    incoming = count
    total_hits = 0

    for level in stats['levels']:
        hits, misses = level['hits'], level['misses']

        if hits < 0 or misses < 0 or hits + misses != incoming:
            raise ValueError(
                f'{config.name}: inconsistent level statistics'
            )

        incoming = misses
        total_hits += hits

    if (
        stats['requests'] != count
        or stats['hits'] != total_hits
        or stats['misses'] != incoming
        or total_hits + incoming != count
        or not 0 <= total_hits <= bound
    ):
        raise ValueError(
            f'{config.name}: inconsistent totals or hits above IDEAL'
        )

    return stats


def main():
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument(
        '--binary',
        type=Path,
        default=ROOT / 'build/cache_runner'
    )
    parser.add_argument(
        '--scenario',
        nargs='+',
        help='Select scenario names from --list'
    )
    parser.add_argument(
        '--list',
        action='store_true',
        help='List available scenarios'
    )
    parser.add_argument(
        '--csv',
        type=Path,
        help='Save all displayed cache rows'
    )
    parser.add_argument(
        '--delimiter',
        choices=[';', ','],
        default=';',
        help='CSV separator (default: semicolon for regional Excel settings)'
    )

    args = parser.parse_args()

    directory = ROOT / 'requests/comparison'
    cases = json.loads((directory / 'scenarios.json').read_text())
    names = {case['name'] for case in cases}

    if args.scenario and set(args.scenario) - names:
        parser.error(
            'Unknown scenarios: '
            + ', '.join(sorted(set(args.scenario) - names))
        )

    if args.scenario:
        cases = [
            case for case in cases
            if case['name'] in args.scenario
        ]

    if args.list:
        for case in cases:
            print(
                f'{case["name"]}: capacity={case["capacity"]}, '
                f'requests={case["requests"]}; {case["description"]}'
            )
        return

    rows = []

    with tempfile.TemporaryDirectory() as temporary:
        configs = {}

        for policy in ('LRU', 'LFU', '2Q', 'ARC', 'LIRS'):
            path = Path(temporary) / f'{policy}.cfg'
            path.write_text(f'1 {policy}\n')
            configs[policy] = path

        multilevel_config = Path(temporary) / 'multilevel.cfg'
        multilevel_config.write_text('2 LRU LFU\n')

        for case in cases:
            request_path = directory / case['file']
            capacity, data = load_requests(request_path)

            if (
                capacity != case['capacity']
                or len(data) != case['requests']
            ):
                raise ValueError(
                    f'{request_path}: metadata does not match input'
                )

            bound = ideal_hits(data, capacity)
            results = []

            for policy, config in configs.items():
                stats = run_cache(
                    args.binary.resolve(),
                    config,
                    request_path,
                    len(data),
                    bound
                )

                results.append(
                    (policy, stats['hits'], stats['misses'], '')
                )

            results.append(
                ('IDEAL', bound, len(data) - bound, '')
            )

            if capacity // 3 >= 1:
                stats = run_cache(
                    args.binary.resolve(),
                    multilevel_config,
                    request_path,
                    len(data),
                    bound
                )

                capacities = [
                    level['capacity'] for level in stats['levels']
                ]

                if capacities != [
                    capacity // 3,
                    capacity - capacity // 3
                ]:
                    raise ValueError(
                        'Unexpected multilevel capacity distribution'
                    )

                level_hits = ' / '.join(
                    str(level['hits']) for level in stats['levels']
                )

                results.append(
                    (
                        'L1+L2 (LRU+LFU)',
                        stats['hits'],
                        stats['misses'],
                        level_hits
                    )
                )

            print(
                f'\nScenario: {case["name"]} — {case["description"]}'
            )
            print(f'Capacity: {capacity}\nRequests: {len(data)}')

            print(
                'Data: '
                + ' '.join(map(str, data[:40]))
                + (' ...' if len(data) > 40 else '')
            )

            if capacity // 3 >= 1:
                print(
                    f'L1 (LRU): {capacity // 3}, '
                    f'L2 (LFU): {capacity - capacity // 3}'
                )

            print(f'\n{"Cache":<20}{"Hits":>10}{"Misses":>10}')

            for policy, hits, misses, level_hits in results:
                print(f'{policy:<20}{hits:>10}{misses:>10}')

                rows.append({
                    'scenario': case['name'],
                    'capacity': capacity,
                    'requests': len(data),
                    'cache': policy,
                    'hits': hits,
                    'misses': misses,
                    'level_hits': level_hits
                })

                if level_hits:
                    print(f'Level hits (L1 / L2): {level_hits}')

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

        print(f'\nSaved: {args.csv}')


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(error.stderr.strip() or str(error), file=sys.stderr)
        sys.exit(1)
    except (OSError, ValueError, KeyError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)