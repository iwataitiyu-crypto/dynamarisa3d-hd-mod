# 黄昏フロンティア(非想天則世代)形式 dat アーカイブの一覧表示/展開
#   python datx.py list  <dat>
#   python datx.py extract <dat> <outdir> [部分一致パターン...]
#   python datx.py pack <dat> <srcdir>   (srcdir 以下のファイルを data/... の名前で格納)
import struct, sys, os

class MT:
    def __init__(self, seed):
        self.mt = [0]*624; self.i = 624
        self.mt[0] = seed & 0xffffffff
        for i in range(1, 624):
            self.mt[i] = (1812433253 * (self.mt[i-1] ^ (self.mt[i-1] >> 30)) + i) & 0xffffffff
    def next(self):
        if self.i >= 624:
            for k in range(624):
                y = (self.mt[k] & 0x80000000) | (self.mt[(k+1) % 624] & 0x7fffffff)
                self.mt[k] = self.mt[(k+397) % 624] ^ (y >> 1) ^ (0x9908b0df if y & 1 else 0)
            self.i = 0
        y = self.mt[self.i]; self.i += 1
        y ^= y >> 11; y ^= (y << 7) & 0x9d2c5680; y ^= (y << 15) & 0xefc60000; y ^= y >> 18
        return y

def read_list(path):
    with open(path, 'rb') as f:
        n, size = struct.unpack('<HI', f.read(6))
        lst = bytearray(f.read(size))
    mt = MT(size + 6)
    for i in range(size): lst[i] ^= mt.next() & 0xff
    k, t = 0xC5, 0x89
    for i in range(size):
        lst[i] ^= k; k = (k + t) & 0xff; t = (t + 0x49) & 0xff
    out, p = [], 0
    for _ in range(n):
        off, sz, nl = struct.unpack_from('<IIB', lst, p); p += 9
        out.append((bytes(lst[p:p+nl]).decode('cp932', 'replace'), off, sz)); p += nl
    return out

def read_file(path, off, sz):
    with open(path, 'rb') as f:
        f.seek(off); d = bytearray(f.read(sz))
    key = ((off >> 1) | 0x23) & 0xff
    for i in range(len(d)): d[i] ^= key
    return bytes(d)

def write_dat(path, files):
    """files: [(アーカイブ内の名前, bytes)]"""
    names = [n.encode('cp932') for n, _ in files]
    size = sum(9 + len(n) for n in names)
    off = 6 + size
    lst = bytearray()
    for n, (_, data) in zip(names, files):
        lst += struct.pack('<IIB', off, len(data), len(n)) + n
        off += len(data)
    k, t = 0xC5, 0x89
    for i in range(size):
        lst[i] ^= k; k = (k + t) & 0xff; t = (t + 0x49) & 0xff
    mt = MT(size + 6)
    for i in range(size): lst[i] ^= mt.next() & 0xff
    with open(path, 'wb') as f:
        f.write(struct.pack('<HI', len(files), size)); f.write(lst)
        for _, data in files:
            key = ((f.tell() >> 1) | 0x23) & 0xff
            f.write(bytes(b ^ key for b in data))

if __name__ == '__main__':
    cmd, dat = sys.argv[1], sys.argv[2]
    if cmd == 'pack':
        src = sys.argv[3]; files = []
        for root, _, fs in os.walk(src):
            for fn in fs:
                full = os.path.join(root, fn)
                files.append((os.path.relpath(full, src).replace(os.sep, '/'), open(full, 'rb').read()))
        write_dat(dat, sorted(files))
        for n, d in sorted(files): print('packed', n, len(d))
        sys.exit(0)
    entries = read_list(dat)
    if cmd == 'list':
        for name, off, sz in entries: print(f'{sz:10d} {name}')
    else:
        outdir, pats = sys.argv[3], sys.argv[4:]
        for name, off, sz in entries:
            if pats and not any(p in name for p in pats): continue
            dst = os.path.join(outdir, name.replace('/', os.sep))
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            with open(dst, 'wb') as f: f.write(read_file(dat, off, sz))
