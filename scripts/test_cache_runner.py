#!/usr/bin/env python3
"""Integration tests. Set CACHE_RUNNER to override build/cache_runner."""

import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
import unittest

BINARY = Path(os.environ.get('CACHE_RUNNER', 'build/cache_runner')).resolve()


class CacheRunnerTests(unittest.TestCase):
    def run_cache(self, config, requests, success=True, json_output=True, stdin=False):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'config').write_text(config)
            (root / 'requests').write_text(requests)
            command = [str(BINARY), str(root / 'config')]
            if not stdin:
                command.append(str(root / 'requests'))
            if json_output:
                command.append('--json')
            result = subprocess.run(command, input=requests if stdin else None,
                                    text=True, capture_output=True)
        if not success:
            self.assertNotEqual(result.returncode, 0)
            self.assertTrue(result.stderr)
            self.assertEqual(result.stdout, '')
            return
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, '')
        if not json_output:
            return result.stdout
        stats = json.loads(result.stdout)
        incoming = stats['requests']
        for level in stats['levels']:
            self.assertEqual(incoming, level['hits'] + level['misses'])
            incoming = level['misses']
        self.assertEqual(incoming, stats['misses'])
        self.assertEqual(stats['hits'], sum(level['hits'] for level in stats['levels']))
        self.assertEqual(stats['requests'], stats['hits'] + stats['misses'])
        return stats

    def test_slide_examples_and_plain_output(self):
        for stdin in (False, True):
            with self.subTest(stdin=stdin):
                self.assertEqual(self.run_cache('1 LRU', '2 4 1 2 1 2',
                                               json_output=False, stdin=stdin), '2\n')
                self.assertEqual(self.run_cache('1 LRU', '2 10 1 2 3 1 2 4 5 6 4 5',
                                               json_output=False, stdin=stdin), '0\n')
        stats = self.run_cache('2 LFU ARC', '2 4 1 2 1 2')
        self.assertEqual(stats['hits'], 2)

    def test_previous_two_level_example(self):
        data = '1 2 1 2 1 2 3 4 5 6 7 8 1 2 1 2 1 2 9 10 11 12 13 14 1 2 1 2 1 2'
        stats = self.run_cache('2 LRU LFU', '6 30 ' + data)
        self.assertEqual(stats['hits'], 16)
        self.assertEqual([level['hits'] for level in stats['levels']], [12, 4])

    def test_arbitrary_depth_against_lru_reference(self):
        generator = random.Random(42)
        data = [generator.randrange(12) for _ in range(300)]
        for total, count in [(6, 1), (7, 3), (25, 5), (0, 3), (1, 8)]:
            with self.subTest(total=total, count=count):
                weights = sum(range(1, count + 1))
                capacities = [total * weight // weights for weight in range(1, count)]
                capacities.append(total - sum(capacities))
                residents = [[] for _ in capacities]
                expected = [0] * len(capacities)
                for key in data:
                    for i, capacity in enumerate(capacities):
                        hit = key in residents[i]
                        if hit:
                            residents[i].remove(key)
                            expected[i] += 1
                        if capacity:
                            residents[i].append(key)
                            if len(residents[i]) > capacity:
                                residents[i].pop(0)
                        if hit:
                            break
                stats = self.run_cache(str(count) + ' LRU' * count,
                                       f'{total} {len(data)} ' + ' '.join(map(str, data)))
                self.assertEqual([level['capacity'] for level in stats['levels']], capacities)
                self.assertEqual([level['hits'] for level in stats['levels']], expected)

    def test_all_policies(self):
        for policy in ['LRU', 'LFU', '2Q', 'ARC', 'LIRS']:
            with self.subTest(policy=policy):
                stats = self.run_cache(f'1 {policy}', '3 3 1 1 1')
                self.assertEqual(stats['hits'], 2)

    def test_empty_requests_and_comments(self):
        stats = self.run_cache('# comment\n2 LRU # L1\nLFU\n', '6 0 # no requests\n')
        self.assertEqual(stats['requests'], 0)

    def test_rejects_invalid_config(self):
        for config in ['', '0', '-1 LRU', '1 IDEAL', '2 LRU', '1 LRU LFU',
                       'LRU 2', '1.5 ARC', '999999999999999999999999999999']:
            with self.subTest(config=config):
                self.run_cache(config, '6 1 1', success=False)

    def test_rejects_invalid_requests(self):
        for data in ['', '2', '-1 0', '1 -1', '1 2 1', '1 1 1 2', '1 0 0',
                     '1 1 -1', '1 1 word', '1 1 1.5',
                     '1 1 999999999999999999999999999999999999']:
            with self.subTest(data=data):
                self.run_cache('1 LRU', data, success=False)

    def test_missing_file(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = str(Path(directory) / 'missing')
            result = subprocess.run([str(BINARY), missing, missing], capture_output=True)
            self.assertNotEqual(result.returncode, 0)


if __name__ == '__main__':
    unittest.main()
