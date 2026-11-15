#!/usr/bin/env python3
"""Generate reproducible read-only workloads; files contain capacity, request count and keys."""

import argparse
from pathlib import Path
import random


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=Path('requests/generated'))
    parser.add_argument('--seed', type=int, default=2026)
    parser.add_argument('--capacity', type=int, default=6)
    args = parser.parse_args()
    if args.capacity < 0:
        parser.error('--capacity must be nonnegative')
    generator = random.Random(args.seed)
    traces = {
        'repeated': [1, 2, 3] * 30,
        'sequential': list(range(1, 121)),
        'cyclic': list(range(1, 8)) * 20,
        'switching_interests': list(range(1, 7)) * 20 + list(range(11, 17)) * 30,
        'moving_window': [key for start in range(1, 21)
                          for _ in range(3) for key in range(start, start + 4)],
        'uniform': [generator.randrange(1, 33) for _ in range(1000)],
    }
    population = [key for key in range(1, 33) for _ in range(320 // key)]
    traces['popular'] = [generator.choice(population) for _ in range(1000)]
    traces['hot_keys'] = [1, 2] * 3
    for start in (3, 9):
        traces['hot_keys'].extend(range(start, start + 6))
        traces['hot_keys'].extend([1, 2] * 3)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, data in traces.items():
        path = args.output_dir / f'{name}.txt'
        with path.open('w', encoding='utf-8') as output:
            output.write(f'# Scenario: {name}; seed: {args.seed}\n')
            output.write(f'{args.capacity} {len(data)}\n')
            for start in range(0, len(data), 30):
                output.write(' '.join(map(str, data[start:start + 30])) + '\n')
        print(f'{path}: {len(data)} requests')


if __name__ == '__main__':
    main()
