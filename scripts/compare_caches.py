#!/usr/bin/env python3
"""Compare saved workloads using single caches and configurable hierarchies."""

import argparse
import csv
import itertools
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
POLICIES = ('LRU', 'LFU', '2Q', 'ARC', 'LIRS')


def ideal_hits(data, capacity):
    """Belady reference for an initially empty cache."""
    if capacity == 0:
        return 0
    next_use = [len(data)] * len(data)
    positions = {}
    for index in range(len(data) - 1, -1, -1):
        next_use[index] = positions.get(data[index], len(data))
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
    values = [int(token) for line in path.read_text(encoding='utf-8').splitlines()
              for token in line.split('#', 1)[0].split()]
    if len(values) < 2 or any(value < 0 for value in values):
        raise ValueError(f'{path}: expected nonnegative capacity, count and keys')
    capacity, count, *data = values
    if len(data) != count:
        raise ValueError(f'{path}: declared request count does not match data')
    return capacity, data


def run_cache(binary, config, requests, count, bound, options=()):
    result = subprocess.run([str(binary), str(config), str(requests), '--json', *options],
                            text=True, capture_output=True, check=True)
    stats = json.loads(result.stdout)
    incoming = count
    total_hits = 0
    for level in stats['levels']:
        hits, misses = level['hits'], level['misses']
        if hits < 0 or misses < 0 or hits + misses != incoming:
            raise ValueError(f'{config.name}: inconsistent level statistics')
        incoming = misses
        total_hits += hits
    if (stats['requests'] != count or stats['hits'] != total_hits
            or stats['misses'] != incoming or total_hits + incoming != count
            or not 0 <= total_hits <= bound):
        raise ValueError(f'{config.name}: inconsistent totals or hits above IDEAL')
    return stats


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build/cache_runner')
    parser.add_argument('--scenario', nargs='+', help='Select names shown by --list')
    parser.add_argument('--list', action='store_true', help='List scenarios without running them')
    parser.add_argument('--csv', type=Path, help='Save the displayed results')
    parser.add_argument('--delimiter', choices=[';', ','], default=';')
    parser.add_argument('--levels', nargs='+', choices=POLICIES, default=['LRU', 'LFU'],
                        help='Hierarchy policies from first to last level (default: LRU LFU)')
    parser.add_argument('--all-pairs', action='store_true', help='Test all 25 ordered policy pairs')
    parser.add_argument('--distribution', choices=['equal', 'linear', 'geometric'], default='geometric')
    parser.add_argument('--ratio', type=int, default=2, help='Geometric factor, at least 2')
    args = parser.parse_args()
    if args.ratio < 2:
        parser.error('--ratio must be at least 2')
    directory = ROOT / 'requests/comparison'
    cases = json.loads((directory / 'scenarios.json').read_text(encoding='utf-8'))
    names = {case['name'] for case in cases}
    if args.scenario and set(args.scenario) - names:
        parser.error('Unknown scenarios: ' + ', '.join(sorted(set(args.scenario) - names)))
    cases = [case for case in cases if not args.scenario or case['name'] in args.scenario]
    if args.list:
        for case in cases:
            print(f'{case["name"]}: capacity={case["capacity"]}, requests={case["requests"]}; '
                  f'{case["description"]}')
        return

    combinations = list(itertools.product(POLICIES, repeat=2)) if args.all_pairs else [args.levels]
    rows = []
    with tempfile.TemporaryDirectory() as temporary:
        configs = {}
        for levels in [*( (p,) for p in POLICIES ), *combinations]:
            levels = tuple(levels)
            path = Path(temporary) / ('_'.join(levels) + '.cfg')
            path.write_text(f'{len(levels)} ' + ' '.join(levels) + '\n', encoding='utf-8')
            configs[levels] = path
        for case in cases:
            path = directory / case['file']
            capacity, data = load_requests(path)
            if capacity != case['capacity'] or len(data) != case['requests']:
                raise ValueError(f'{path}: metadata does not match input')
            bound = ideal_hits(data, capacity)
            print(f'\n{case["name"]}: capacity={capacity}, requests={len(data)}')
            print(f'{"Cache":<24} {"Mode":<12} {"Hits":>7} {"Misses":>7}  Sizes / level hits')

            def record(label, mode, stats, distribution='', ratio=''):
                sizes = '/'.join(str(level['capacity']) for level in stats['levels'])
                hits = '/'.join(str(level['hits']) for level in stats['levels'])
                rows.append(dict(scenario=case['name'], capacity=capacity, requests=len(data),
                                 cache=label, mode=mode, distribution=distribution, ratio=ratio,
                                 hits=stats['hits'], misses=stats['misses'],
                                 level_sizes=sizes, level_hits=hits))
                print(f'{label:<24} {mode:<12} {stats["hits"]:>7} {stats["misses"]:>7}  {sizes} / {hits}')

            for policy in POLICIES:
                stats = run_cache(args.binary.resolve(), configs[(policy,)], path, len(data), bound)
                record(policy, 'single', stats)
            record('IDEAL', 'offline', dict(hits=bound, misses=len(data)-bound, levels=[]))
            for levels in combinations:
                count = len(levels)
                weights = ([1] * count if args.distribution == 'equal' else
                           list(range(1, count + 1)) if args.distribution == 'linear' else
                           [args.ratio ** i for i in range(count)])
                if count > 1 and capacity // sum(weights) == 0:
                    print(f'{"+".join(levels)}: skipped (allocation would give an empty level)')
                    continue
                options = ['--distribution', args.distribution]
                if args.distribution == 'geometric':
                    options += ['--ratio', str(args.ratio)]
                for mode in ['independent']:
                    stats = run_cache(args.binary.resolve(), configs[tuple(levels)], path,
                                      len(data), bound, options)
                    record('+'.join(levels), mode, stats, args.distribution,
                           args.ratio if args.distribution == 'geometric' else '')
    if args.csv:
        args.csv.parent.mkdir(parents=True, exist_ok=True)
        with args.csv.open('w', newline='', encoding='utf-8-sig') as output:
            writer = csv.DictWriter(output, fieldnames=rows[0].keys(), delimiter=args.delimiter)
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
