#!/usr/bin/env python3
"""Integration tests for the CLI, input files, output and hierarchy statistics."""

import argparse
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile
import unittest

BINARY = Path('build/cache_runner').resolve()


class CacheRunnerTests(unittest.TestCase):
    def run_cache(self, config, requests, options=(), success=True, json_output=True, stdin=False):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'config').write_text(config, encoding='utf-8')
            (root / 'requests').write_text(requests, encoding='utf-8')
            command = [str(BINARY), str(root / 'config')]
            if not stdin:
                command.append(str(root / 'requests'))
            command.extend(options)
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

    def test_plain_output_and_stdin(self):
        for stdin in (False, True):
            self.assertEqual(self.run_cache('1 LRU', '2 4 1 2 1 2',
                                           json_output=False, stdin=stdin), '2\n')

    def test_previous_independent_result(self):
        data = '1 2 1 2 1 2 3 4 5 6 7 8 1 2 1 2 1 2 9 10 11 12 13 14 1 2 1 2 1 2'
        stats = self.run_cache('2 LRU LFU', '6 30 ' + data)
        self.assertEqual(stats['hits'], 16)
        self.assertEqual([level['hits'] for level in stats['levels']], [12, 4])

    def test_distribution_options(self):
        for options, expected in [([], [3, 6, 12]),
                                  (['--distribution', 'equal'], [7, 7, 7]),
                                  (['--distribution', 'linear'], [3, 7, 11]),
                                  (['--ratio', '4'], [1, 4, 16]),
                                  (['--sizes', '2,5,14'], [2, 5, 14])]:
            stats = self.run_cache('3 LRU ARC LIRS', '21 0', options)
            self.assertEqual([level['capacity'] for level in stats['levels']], expected)

    def test_all_policies(self):
        for policy in ['LRU', 'LFU', '2Q', 'ARC', 'LIRS']:
            self.assertEqual(self.run_cache(f'1 {policy}', '3 3 1 1 1')['hits'], 2)
            self.assertEqual(self.run_cache(f'1 {policy}', '0 3 1 1 1')['hits'], 0)

    def test_empty_requests_and_existing_comment_format(self):
        self.assertEqual(self.run_cache('# comment\n2 LRU # L1\nLFU\n',
                                       '6 0 # no requests\n')['requests'], 0)

    def test_rejects_invalid_config(self):
        for config in ['', '0', '-1 LRU', '1 IDEAL', '2 LRU', '1 LRU LFU',
                       'LRU 2', '1.5 ARC', '999999999999999999999999999999']:
            self.run_cache(config, '6 1 1', success=False)

    def test_rejects_invalid_requests(self):
        for data in ['', '2', '-1 0', '1 -1', '1 2 1', '1 1 1 2', '1 0 0',
                     '1 1 -1', '1 1 word', '1 1 1.5',
                     '1 1 999999999999999999999999999999999999']:
            self.run_cache('1 LRU', data, success=False)

    def test_rejects_invalid_options(self):
        for options in [['--mode', 'other'], ['--ratio', '1'], ['--ratio', '-2'],
                        ['--ratio', '2', '--distribution', 'equal'],
                        ['--sizes', '2,2'], ['--sizes', '-1,7'], ['--sizes', '0,6'],
                        ['--sizes', '2,4', '--distribution', 'equal'],
                        ['--unknown'], ['--distribution', 'other']]:
            self.run_cache('2 LRU ARC', '6 0', options, success=False)
        self.run_cache('2 LRU ARC', '2 0', success=False)
        self.run_cache('3 LRU ARC LFU', '6 0', success=False)

    def test_help(self):
        result = subprocess.run([str(BINARY), '--help'], text=True, capture_output=True)
        self.assertEqual(result.returncode, 0)
        for option in ['--json', '--distribution', '--sizes', '--ratio']:
            self.assertIn(option, result.stdout)

    def test_missing_file(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([str(BINARY), str(Path(directory) / 'missing')],
                                    capture_output=True)
            self.assertNotEqual(result.returncode, 0)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=BINARY, help='Built cache_runner executable')
    args, remaining = parser.parse_known_args()
    BINARY = args.binary.resolve()
    unittest.main(argv=[sys.argv[0], *remaining])
