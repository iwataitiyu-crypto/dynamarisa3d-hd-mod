# Squirrel 2.x のコンパイル済みスクリプト (.cv4 / .cnut) の逆アセンブラ
#   python sqdis.py <file.cv4>
import struct, sys

OT_NULL, OT_INTEGER, OT_FLOAT, OT_BOOL, OT_STRING = 0x01000001, 0x05000002, 0x05000004, 0x01000008, 0x08000010

OPS = ['LINE', 'LOAD', 'LOADINT', 'LOADFLOAT', 'DLOAD', 'TAILCALL', 'CALL', 'PREPCALL', 'PREPCALLK', 'GETK',
       'MOVE', 'NEWSLOT', 'DELETE', 'SET', 'GET', 'EQ', 'NE', 'ARITH', 'BITW', 'RETURN', 'LOADNULLS',
       'LOADROOTTABLE', 'LOADBOOL', 'DMOVE', 'JMP', 'JNZ', 'JZ', 'LOADFREEVAR', 'VARGC', 'GETVARGV',
       'NEWTABLE', 'NEWARRAY', 'APPENDARRAY', 'GETPARENT', 'COMPARITH', 'COMPARITHL', 'INC', 'INCL',
       'PINC', 'PINCL', 'CMP', 'EXISTS', 'INSTANCEOF', 'AND', 'OR', 'NEG', 'NOT', 'BWNOT', 'CLOSURE',
       'YIELD', 'RESUME', 'FOREACH', 'POSTFOREACH', 'DELEGATE', 'CLONE', 'TYPEOF', 'PUSHTRAP', 'POPTRAP',
       'THROW', 'CLASS', 'NEWSLOTA']


class Reader:
    def __init__(self, data):
        self.d, self.p = data, 0

    def i32(self):
        v = struct.unpack_from('<i', self.d, self.p)[0]; self.p += 4; return v

    def u32(self):
        v = struct.unpack_from('<I', self.d, self.p)[0]; self.p += 4; return v

    def tag(self, t=b'TRAP'):
        assert self.d[self.p:self.p + 4] == t, (self.p, self.d[self.p:self.p + 4]); self.p += 4

    def obj(self):
        """(型, 値, 値のファイル内オフセット)"""
        t = self.u32()
        pos = self.p
        if t == OT_STRING:
            n = self.i32(); s = self.d[self.p:self.p + n]; self.p += n
            return ('str', s.decode('cp932', 'replace'), pos)
        if t == OT_INTEGER: return ('int', self.i32(), pos)
        if t == OT_FLOAT:
            v = struct.unpack_from('<f', self.d, self.p)[0]; self.p += 4; return ('float', v, pos)
        if t == OT_BOOL: return ('bool', self.i32(), pos)
        if t == OT_NULL: return ('null', None, pos)
        raise ValueError('unknown object type %x at %d' % (t, self.p))


class Func:
    pass


def read_func(r):
    f = Func()
    r.tag()
    f.source, f.name = r.obj()[1], r.obj()[1]
    r.tag()
    nlit, npar, nout, nloc, nline, ndef, nins, nfun = [r.i32() for _ in range(8)]
    r.tag(); f.literals = [r.obj() for _ in range(nlit)]
    r.tag(); f.params = [r.obj()[1] for _ in range(npar)]
    r.tag(); f.outers = []
    for _ in range(nout):
        t = r.u32(); src = r.obj(); name = r.obj(); f.outers.append((t, src[1], name[1]))
    r.tag(); f.locals = []
    for _ in range(nloc):
        name = r.obj()[1]; pos, s, e = r.i32(), r.i32(), r.i32(); f.locals.append((name, pos, s, e))
    r.tag(); f.lines = [struct.unpack_from('<ii', r.d, r.p + 8 * i) for i in range(nline)]; r.p += 8 * nline
    r.tag(); f.defaults = [r.i32() for _ in range(ndef)]
    r.tag(); f.ins_pos = r.p; f.ins = []
    for _ in range(nins):
        a1, op, a0, a2, a3 = struct.unpack_from('<iBBBB', r.d, r.p); r.p += 8; f.ins.append((op, a0, a1, a2, a3))
    r.tag(); f.funcs = [read_func(r) for _ in range(nfun)]
    # _stacksize (int) と _bgenerator / _varparams (各 bool 1 バイト)
    f.stacksize = r.i32()
    f.generator, f.varparams = r.d[r.p], r.d[r.p + 1]
    r.p += 2
    return f


def load(data):
    r = Reader(data)
    assert data[:2] == b'\xfa\xfa'
    r.p = 2
    r.tag(b'RIQS')
    sz_char = r.i32()  # sizeof(SQChar)
    f = read_func(r)
    return f


def lit(f, i):
    if 0 <= i < len(f.literals):
        k, v, _ = f.literals[i]
        return repr(v) if k == 'str' else str(v)
    return '?%d' % i


def dump(f, depth=0, out=sys.stdout):
    ind = '  ' * depth
    out.write('%sfunction %s(%s)  [stack %d]\n' % (ind, f.name, ', '.join(map(str, f.params)), f.stacksize))
    for i, (op, a0, a1, a2, a3) in enumerate(f.ins):
        name = OPS[op] if op < len(OPS) else 'OP%02x' % op
        extra = ''
        if name in ('LOAD', 'PREPCALLK', 'GETK', 'DLOAD'):
            extra = ' ; ' + lit(f, a1)
            if name == 'DLOAD': extra += ' , ' + lit(f, a3)
        elif name == 'LOADFLOAT':
            extra = ' ; %g' % struct.unpack('<f', struct.pack('<i', a1))[0]
        elif name == 'CLOSURE':
            extra = ' ; ' + (f.funcs[a1].name if a1 < len(f.funcs) else '?')
        out.write('%s  %4d %-12s %3d %6d %3d %3d%s\n' % (ind, i, name, a0, a1, a2, a3, extra))
    for g in f.funcs:
        dump(g, depth + 1, out)


if __name__ == '__main__':
    data = open(sys.argv[1], 'rb').read()
    f = load(data)
    sys.stdout.reconfigure(encoding='utf-8')
    dump(f)
