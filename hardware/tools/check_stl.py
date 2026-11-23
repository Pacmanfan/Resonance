#!/usr/bin/env python3
"""Check binary STL files: triangle count, bounding box, edge manifoldness,
connected components, signed volume. Needs numpy only."""
import sys, struct, os
import numpy as np

def load(path):
    with open(path, 'rb') as f:
        data = f.read()
    n = struct.unpack('<I', data[80:84])[0]
    assert len(data) == 84 + 50 * n, "not a binary STL (size mismatch)"
    rec = np.frombuffer(data, dtype=np.dtype([('n', '<f4', 3), ('v', '<f4', (3, 3)), ('a', '<u2')]), offset=84, count=n)
    return rec['v'].astype(np.float64)

def check(path):
    tri = load(path)
    n = len(tri)
    pts = tri.reshape(-1, 3)
    uniq, idx = np.unique(pts, axis=0, return_inverse=True)   # exact match: OpenSCAD writes identical floats
    f = idx.reshape(-1, 3)
    degenerate = int(np.sum((f[:, 0] == f[:, 1]) | (f[:, 1] == f[:, 2]) | (f[:, 0] == f[:, 2])))
    # directed edges
    e = np.concatenate([f[:, [0, 1]], f[:, [1, 2]], f[:, [2, 0]]])
    und = np.sort(e, axis=1)
    _, cnt = np.unique(und, axis=0, return_counts=True)
    bad_edges = int(np.sum(cnt != 2))
    # orientation: every directed edge must occur once, with its reverse once
    key = e[:, 0] * len(uniq) + e[:, 1]
    rkey = e[:, 1] * len(uniq) + e[:, 0]
    oriented = (len(np.unique(key)) == len(key)) and np.array_equal(np.sort(key), np.sort(rkey))
    # components (union-find over faces via vertices)
    parent = np.arange(len(uniq))
    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]; a = parent[a]
        return a
    for a, b in und:
        ra, rb = find(a), find(b)
        if ra != rb: parent[ra] = rb
    comps = len({find(i) for i in range(len(uniq))})
    vol = float(np.sum(np.einsum('ij,ij->i', tri[:, 0], np.cross(tri[:, 1], tri[:, 2]))) / 6.0)
    mn, mx = pts.min(0), pts.max(0)
    return dict(file=os.path.basename(path), bytes=os.path.getsize(path), tris=n, verts=len(uniq),
                degenerate=degenerate, bad_edges=bad_edges, oriented=oriented, comps=comps,
                euler=len(uniq) - len(cnt) + n, vol=vol, mn=mn, size=mx - mn)

if __name__ == '__main__':
    ok = True
    print("| File | Bytes | Triangles | Bounding box X x Y x Z (mm) | Min corner | Edges not shared by 2 | Consistent orientation | Shells | Volume (mm^3) |")
    print("|---|---|---|---|---|---|---|---|---|")
    for p in sys.argv[1:]:
        r = check(p)
        good = r['tris'] > 0 and r['bad_edges'] == 0 and r['oriented'] and r['vol'] > 0 and r['degenerate'] == 0
        ok &= good
        print("| %s | %d | %d | %.2f x %.2f x %.2f | %.2f, %.2f, %.2f | %d | %s | %d | %.1f |" % (
            r['file'], r['bytes'], r['tris'], *r['size'], *r['mn'], r['bad_edges'], 'yes' if r['oriented'] else 'NO', r['comps'], r['vol']))
    sys.exit(0 if ok else 1)
