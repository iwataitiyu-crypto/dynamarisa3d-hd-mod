# d3d9.h の IDirect3D9 / IDirect3DDevice9 / IDirect3DSwapChain9 から素通しラッパー(wrap_gen.h)を生成する
#   python tools/gen_wrappers.py src/wrap_gen.h
import glob, re, sys
# インストールされている Windows SDK のうち最新の d3d9.h を使う
HDR = sorted(glob.glob(r"C:\Program Files (x86)\Windows Kits\10\Include\*\shared\d3d9.h"))[-1]
hdr = open(HDR, encoding="utf-8", errors="ignore").read()

def methods(iface):
    body = re.search(r"DECLARE_INTERFACE_\(%s, IUnknown\)\s*\{(.*?)\n\};" % iface, hdr, re.S).group(1)
    out = []
    for m in re.finditer(r"STDMETHOD(_?)\(([^)]*)\)\(THIS_?\s*(.*?)\)\s*PURE;", body, re.S):
        if m.group(1):
            ret, name = [x.strip() for x in m.group(2).split(",")]
        else:
            ret, name = "HRESULT", m.group(2).strip()
        out.append((ret, name, " ".join(m.group(3).split())))
    return out

def split_args(args):
    if not args: return []
    res, cur = [], ""
    for ch in args:
        if ch == ",":
            res.append(cur.strip()); cur = ""
        else:
            cur += ch
    if cur.strip(): res.append(cur.strip())
    out = []
    for i, a in enumerate(res):
        a = re.sub(r"\[.*?\]", "", a)
        if a.endswith("*") or len(a.split()) == 1 or a.split()[-1] == "D3DTRANSFORMSTATETYPE":
            a += " unnamed%d" % i
        out.append((a, re.findall(r"(\w+)\s*$", a)[0]))
    return out

def gen(iface):
    lines = []
    for ret, name, args in methods(iface):
        if name in ("QueryInterface", "AddRef", "Release"): continue
        pa = split_args(args)
        head = ("STDMETHOD_(%s, %s)" % (ret, name)) if ret != "HRESULT" else ("STDMETHOD(%s)" % name)
        decl = ", ".join(a for a, _ in pa)
        call = ", ".join(n for _, n in pa)
        lines.append("    %s(THIS%s%s) override { return real->%s(%s); }" % (head, "_ " if pa else "", decl, name, call))
    return "\n".join(lines)

with open(sys.argv[1], "w", encoding="utf-8") as f:
    f.write("// 自動生成: gen_wrappers.py\n#pragma once\n#include <d3d9.h>\n\n")
    for iface, cls in (("IDirect3D9", "D3D9Base"), ("IDirect3DDevice9", "DeviceBase"), ("IDirect3DSwapChain9", "SwapChainBase")):
        f.write("struct %s : %s {\n    %s* real;\n    %s(%s* r) : real(r) {}\n" % (cls, iface, iface, cls, iface))
        f.write("    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppv) override { return real->QueryInterface(riid, ppv); }\n")
        f.write("    STDMETHOD_(ULONG, AddRef)(THIS) override { return real->AddRef(); }\n")
        f.write("    STDMETHOD_(ULONG, Release)(THIS) override { ULONG r = real->Release(); if (r == 0) delete this; return r; }\n")
        f.write(gen(iface) + "\n};\n\n")
