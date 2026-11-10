// 調査用: D3D9 呼び出しをログに記録するだけのプロキシ
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "wrap_gen.h"

HMODULE g_realD3D9;
#include "forward.h"
static FILE* g_log;
static void L(const char* f, ...) {
    if (!g_log) g_log = fopen("d3d9probe.log", "w");
    va_list a; va_start(a, f); vfprintf(g_log, f, a); va_end(a); fputc('\n', g_log); fflush(g_log);
}
static int g_frame;
static bool g_trace;
static int g_cnt[16];  // 0 DP 1 DIP 2 DPUP 3 DIPUP 4 clear 5 setRT
static DWORD g_fvf; static bool g_vs, g_ps; static int g_rtw, g_rth;

static void dumpPP(const char* tag, D3DPRESENT_PARAMETERS* p) {
    L("%s: BB %ux%u fmt=%d count=%u MS=%d swap=%d hwnd=%p windowed=%d autoDS=%d DSfmt=%d flags=0x%lx interval=0x%x",
      tag, p->BackBufferWidth, p->BackBufferHeight, p->BackBufferFormat, p->BackBufferCount, p->MultiSampleType,
      p->SwapEffect, p->hDeviceWindow, p->Windowed, p->EnableAutoDepthStencil, p->AutoDepthStencilFormat, p->Flags, p->PresentationInterval);
}
static int g_traceAt = 600;
static void onPresent(const char* who) {
    if (g_trace) { L("=== end trace (%s)", who); g_trace = false; }
    if (g_frame % 300 == 0) L("[%s] frame %d: DP=%d DIP=%d DPUP=%d DIPUP=%d clear=%d setRT=%d", who, g_frame, g_cnt[0], g_cnt[1], g_cnt[2], g_cnt[3], g_cnt[4], g_cnt[5]);
    memset(g_cnt, 0, sizeof g_cnt);
    g_frame++;
    if (GetAsyncKeyState(VK_F11) & 1) g_traceAt = g_frame + 1;
    if (g_frame == g_traceAt) { g_trace = true; L("=== begin trace frame %d", g_frame); }
}
#define T(...) do { if (g_trace) L(__VA_ARGS__); } while (0)

struct SC : SwapChainBase {
    using SwapChainBase::SwapChainBase;
    STDMETHOD(Present)(THIS_ CONST RECT* a, CONST RECT* b, HWND w, CONST RGNDATA* c, DWORD f) override { onPresent("SC"); return real->Present(a, b, w, c, f); }
};

struct Dev : DeviceBase {
    using DeviceBase::DeviceBase;
    STDMETHOD(GetSwapChain)(THIS_ UINT i, IDirect3DSwapChain9** o) override {
        L("GetSwapChain %u", i); HRESULT hr = real->GetSwapChain(i, o); if (SUCCEEDED(hr)) *o = new SC(*o); return hr; }
    STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* p, IDirect3DSwapChain9** o) override {
        dumpPP("CreateAdditionalSwapChain", p); HRESULT hr = real->CreateAdditionalSwapChain(p, o); if (SUCCEEDED(hr)) *o = new SC(*o); return hr; }
    STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* p) override { dumpPP("Reset", p); return real->Reset(p); }
    STDMETHOD(Present)(THIS_ CONST RECT* a, CONST RECT* b, HWND w, CONST RGNDATA* c) override {
        if (g_frame % 300 == 0) L("Present src=%p dst=%p hwnd=%p", a, b, w);
        onPresent("Dev"); return real->Present(a, b, w, c); }
    STDMETHOD(GetBackBuffer)(THIS_ UINT a, UINT b, D3DBACKBUFFER_TYPE t, IDirect3DSurface9** o) override { L("GetBackBuffer"); return real->GetBackBuffer(a, b, t, o); }
    STDMETHOD(GetRenderTarget)(THIS_ DWORD i, IDirect3DSurface9** o) override { T("  GetRenderTarget %lu", i); return real->GetRenderTarget(i, o); }
    STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* v) override { T("  GetViewport"); return real->GetViewport(v); }
    STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* v) override {
        T("  SetViewport %lu,%lu %lux%lu z=%g..%g", v->X, v->Y, v->Width, v->Height, v->MinZ, v->MaxZ); return real->SetViewport(v); }
    STDMETHOD(SetRenderTarget)(THIS_ DWORD i, IDirect3DSurface9* s) override {
        g_cnt[5]++;
        D3DSURFACE_DESC d = {}; if (s) s->GetDesc(&d); if (i == 0) { g_rtw = d.Width; g_rth = d.Height; }
        T("SetRenderTarget %lu %ux%u fmt=%d", i, d.Width, d.Height, d.Format); return real->SetRenderTarget(i, s); }
    STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* s) override {
        D3DSURFACE_DESC d = {}; if (s) s->GetDesc(&d); T("  SetDepthStencil %ux%u", d.Width, d.Height); return real->SetDepthStencilSurface(s); }
    STDMETHOD(SetScissorRect)(THIS_ CONST RECT* r) override { T("  SetScissor %ld,%ld,%ld,%ld", r->left, r->top, r->right, r->bottom); return real->SetScissorRect(r); }
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE s, DWORD v) override { if (s == D3DRS_SCISSORTESTENABLE) T("  scissor enable %lu", v); return real->SetRenderState(s, v); }
    STDMETHOD(SetFVF)(THIS_ DWORD f) override { g_fvf = f; T("  SetFVF 0x%lx", f); return real->SetFVF(f); }
    STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* s) override { g_vs = s != 0; T("  SetVS %p", s); return real->SetVertexShader(s); }
    STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* s) override { g_ps = s != 0; T("  SetPS %p", s); return real->SetPixelShader(s); }
    STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* s) override { T("  SetVDecl %p", s); return real->SetVertexDeclaration(s); }
    STDMETHOD(SetStreamSource)(THIS_ UINT n, IDirect3DVertexBuffer9* vb, UINT o, UINT st) override { T("  SetStream %u vb=%p off=%u stride=%u", n, vb, o, st); return real->SetStreamSource(n, vb, o, st); }
    STDMETHOD(Clear)(THIS_ DWORD n, CONST D3DRECT* r, DWORD f, D3DCOLOR c, float z, DWORD st) override { g_cnt[4]++; T("  Clear n=%lu flags=0x%lx", n, f); return real->Clear(n, r, f, c, z, st); }
    STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE t, UINT c, CONST void* d, UINT s) override {
        g_cnt[2]++; T("  DPUP type=%d n=%u stride=%u fvf=0x%lx vs=%d ps=%d rt=%dx%d", t, c, s, g_fvf, g_vs, g_ps, g_rtw, g_rth);
        if (g_trace) { const float* f = (const float*)d; T("     v0 = %g,%g,%g,%g", f[0], f[1], f[2], f[3]); }
        return real->DrawPrimitiveUP(t, c, d, s); }
    STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE t, UINT a, UINT b, UINT c, CONST void* e, D3DFORMAT f, CONST void* g, UINT h) override {
        g_cnt[3]++; T("  DIPUP type=%d n=%u stride=%u fvf=0x%lx vs=%d ps=%d", t, c, h, g_fvf, g_vs, g_ps); return real->DrawIndexedPrimitiveUP(t, a, b, c, e, f, g, h); }
    STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE t, UINT a, UINT b) override {
        g_cnt[0]++; T("  DP type=%d n=%u fvf=0x%lx vs=%d ps=%d rt=%dx%d", t, b, g_fvf, g_vs, g_ps, g_rtw, g_rth); return real->DrawPrimitive(t, a, b); }
    STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE t, INT a, UINT b, UINT c, UINT d, UINT e) override {
        g_cnt[1]++; T("  DIP type=%d n=%u fvf=0x%lx vs=%d ps=%d rt=%dx%d", t, e, g_fvf, g_vs, g_ps, g_rtw, g_rth); return real->DrawIndexedPrimitive(t, a, b, c, d, e); }
    STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* a, CONST RECT* ra, IDirect3DSurface9* b, CONST RECT* rb, D3DTEXTUREFILTERTYPE f) override {
        T("  StretchRect"); return real->StretchRect(a, ra, b, rb, f); }
    STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* a, IDirect3DSurface9* b) override { L("GetRenderTargetData"); return real->GetRenderTargetData(a, b); }
    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT r, CONST float* d, UINT n) override {
        T("  VSConstF r=%u n=%u [%g %g %g %g]", r, n, d[0], d[1], d[2], d[3]); return real->SetVertexShaderConstantF(r, d, n); }
    STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT r, CONST float* d, UINT n) override {
        T("  PSConstF r=%u n=%u [%g %g %g %g]", r, n, d[0], d[1], d[2], d[3]); return real->SetPixelShaderConstantF(r, d, n); }
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE s, CONST D3DMATRIX* m) override { T("  SetTransform %d", s); return real->SetTransform(s, m); }
    STDMETHOD(SetTexture)(THIS_ DWORD st, IDirect3DBaseTexture9* t) override {
        if (g_trace && t && t->GetType() == D3DRTYPE_TEXTURE) { D3DSURFACE_DESC d; ((IDirect3DTexture9*)t)->GetLevelDesc(0, &d); T("  SetTexture %lu %ux%u usage=0x%lx", st, d.Width, d.Height, d.Usage); }
        return real->SetTexture(st, t); }
};

struct D3D : D3D9Base {
    using D3D9Base::D3D9Base;
    STDMETHOD(CreateDevice)(THIS_ UINT ad, D3DDEVTYPE t, HWND hw, DWORD bf, D3DPRESENT_PARAMETERS* p, IDirect3DDevice9** out) override {
        L("CreateDevice adapter=%u type=%d hwnd=%p behavior=0x%lx", ad, t, hw, bf);
        dumpPP("CreateDevice", p);
        HRESULT hr = real->CreateDevice(ad, t, hw, bf, p, out);
        if (SUCCEEDED(hr)) *out = new Dev(*out);
        return hr;
    }
};

typedef IDirect3D9* (WINAPI* PFN)(UINT);
extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT v) {
    char path[MAX_PATH]; GetSystemDirectoryA(path, MAX_PATH); strcat(path, "\\d3d9.dll");
    static HMODULE h = (g_realD3D9 = LoadLibraryA(path));
    static bool once = (initForwards(), true);
    IDirect3D9* r = ((PFN)GetProcAddress(h, "Direct3DCreate9"))(v);
    L("Direct3DCreate9(%u)", v);
    return r ? new D3D(r) : nullptr;
}
