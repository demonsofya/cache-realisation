#!/usr/bin/env python3
"""Build cache_runner with a C++23 compiler; CMake and GoogleTest are not needed."""

import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'g++'))
    parser.add_argument('--output', type=Path, default=root / 'build' / 'cache_runner')
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([args.cxx, '-std=c++23', '-O2', '-Wall', '-Wextra', '-pedantic',
                    '-I', str(root / 'include'), str(root / 'src/cache_runner.cpp'),
                    str(root / 'src/cache_hierarchy.cpp'), '-o', str(args.output)], check=True)
    print(f'Built: {args.output}')


if __name__ == '__main__':
    try:
        main()
    except (OSError, subprocess.CalledProcessError) as error:
        print(f'Build failed: {error}', file=sys.stderr)
        sys.exit(1)
