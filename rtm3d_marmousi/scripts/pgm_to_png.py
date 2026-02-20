#!/usr/bin/env python3
from pathlib import Path
import sys


def read_pgm(path: Path):
    with path.open('rb') as f:
        magic = f.readline().strip()
        if magic != b'P5':
            raise RuntimeError('expected P5 pgm')
        dims = f.readline().strip()
        while dims.startswith(b'#'):
            dims = f.readline().strip()
        w, h = map(int, dims.split())
        maxv = int(f.readline().strip())
        data = f.read()
    return w, h, maxv, data


def main():
    if len(sys.argv) != 3:
        print('usage: pgm_to_png.py in.pgm out.png')
        return 1
    inp, out = Path(sys.argv[1]), Path(sys.argv[2])
    w, h, _maxv, data = read_pgm(inp)

    try:
        from PIL import Image
    except Exception as e:
        raise RuntimeError('Pillow is required: pip install pillow') from e

    im = Image.frombytes('L', (w, h), data)
    im.save(out)
    print(out)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
