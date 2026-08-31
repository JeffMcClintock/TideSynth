#!/usr/bin/env python3
"""Pixel-diff two P6 PPMs. E19's screen-level check.

    python3 ppmdiff.py a.ppm b.ppm [--tile X Y W H] [--threshold N]

Prints total pixels compared, pixels differing by more than --threshold on any
channel, and the same counts restricted to --tile (the scope display region).
Stdlib only -- there is no numpy on this box.
"""
import sys


def read_ppm(path):
    with open(path, "rb") as f:
        assert f.readline().strip() == b"P6", path + " is not P6"
        line = f.readline()
        while line.startswith(b"#"):
            line = f.readline()
        w, h = map(int, line.split())
        f.readline()
        return w, h, f.read(w * h * 3)


def main():
    a, b = sys.argv[1], sys.argv[2]
    tile = None
    thr = 8
    args = sys.argv[3:]
    while args:
        if args[0] == "--tile":
            tile = tuple(int(v) for v in args[1:5]); args = args[5:]
        elif args[0] == "--threshold":
            thr = int(args[1]); args = args[2:]
        else:
            raise SystemExit("bad arg " + args[0])

    aw, ah, ad = read_ppm(a)
    bw, bh, bd = read_ppm(b)
    if (aw, ah) != (bw, bh):
        raise SystemExit("size mismatch %dx%d vs %dx%d" % (aw, ah, bw, bh))

    total = changed = 0
    t_total = t_changed = 0
    tx, ty, tw, th = tile if tile else (0, 0, 0, 0)
    for y in range(ah):
        row = y * aw * 3
        in_ty = tile and ty <= y < ty + th
        for x in range(aw):
            i = row + x * 3
            d = max(abs(ad[i] - bd[i]), abs(ad[i + 1] - bd[i + 1]), abs(ad[i + 2] - bd[i + 2]))
            total += 1
            hit = d > thr
            if hit:
                changed += 1
            if in_ty and tx <= x < tx + tw:
                t_total += 1
                if hit:
                    t_changed += 1

    print("size %dx%d threshold %d" % (aw, ah, thr))
    print("changed %d of %d" % (changed, total))
    if tile:
        print("tile %d,%d %dx%d: changed %d of %d" % (tx, ty, tw, th, t_changed, t_total))


if __name__ == "__main__":
    main()
