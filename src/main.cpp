// ダイナマリサ3D 高解像度化 MOD (d3d9.dll プロキシ)
//
// ゲームは 1280x720 固定のバックバッファと、同サイズ(およびその 1/2, 1/4...)の
// レンダーターゲットで描画している。これらを設定した倍率で拡大して作成し、
// それに合わせてビューポート・シザー・Clear矩形・XYZRHW(画面座標)頂点を拡大する。
// ゲーム側からは従来どおり 1280x720 の世界に見えるので、スクリプト等は無改造で動く。
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <vector>
#include <map>
#include <algorithm>
#include <unordered_map>
#include <string>
#include "wrap_gen.h"

HMODULE g_realD3D9;
#include "forward.h"

// ---------------------------------------------------------------- 設定 / ログ
struct Config {
    int renderWidth, renderHeight;  // 内部描画解像度 (0 = モニター解像度)
    float supersample;              // 内部解像度に掛ける倍率 (SSAA)
    int shadowScale;                // 影マップ(1024x1024)の倍率
    int anisotropy;                 // 異方性フィルタ (0/1 = 無効)
    int frameLimit;                 // 0 = モニターに合わせる(垂直同期), -1 = 無制限, それ以外 = 上限 fps
    int interpolation;              // フレーム補間 (ロジック 60Hz の間を補間して描画する)
    float dofStrength;              // 被写界深度(遠景ぼかし)の強さ 0 = なし, 1 = オリジナル
    int effectScale;                // グロー等の縮小バッファも拡大する
    int windowWidth, windowHeight;  // ウィンドウモード時のクライアントサイズ (0 = 画面に収まる最大)
    int fixFullscreen;              // ゲームの全画面化(ボーダレス)の位置ずれを補正する
    int aimSmoothing;               // 照準(カメラ回転)の慣性 1 = オリジナル, 0 = なし
    int aimPrecision;               // マウス照準の細かさ (1カウントあたりの回転量を 1/N にする)
    int log;
    int dumpShaders;                // シェーダーの逆アセンブルを shaderdump\ に出力し、定期的に1フレームの描画を記録する
} g_cfg;

static char g_dir[MAX_PATH];
static FILE* g_log;
static void L(const char* f, ...) {
    if (!g_cfg.log) return;
    if (!g_log) { char p[MAX_PATH]; sprintf(p, "%sd3d9mod.log", g_dir); g_log = fopen(p, "w"); if (!g_log) return; }
    va_list a; va_start(a, f); vfprintf(g_log, f, a); va_end(a); fputc('\n', g_log); fflush(g_log);
}

static void loadConfig() {
    GetModuleFileNameA(NULL, g_dir, MAX_PATH);
    *(strrchr(g_dir, '\\') + 1) = 0;
    char ini[MAX_PATH], buf[64];
    sprintf(ini, "%sd3d9mod.ini", g_dir);
    g_cfg.renderWidth = GetPrivateProfileIntA("Graphics", "RenderWidth", 0, ini);
    g_cfg.renderHeight = GetPrivateProfileIntA("Graphics", "RenderHeight", 0, ini);
    GetPrivateProfileStringA("Graphics", "Supersample", "1.0", buf, sizeof buf, ini);
    g_cfg.supersample = (float)atof(buf);
    if (g_cfg.supersample < 0.25f) g_cfg.supersample = 1.0f;
    g_cfg.shadowScale = GetPrivateProfileIntA("Graphics", "ShadowScale", 2, ini);
    if (g_cfg.shadowScale < 1) g_cfg.shadowScale = 1;
    g_cfg.anisotropy = GetPrivateProfileIntA("Graphics", "Anisotropy", 16, ini);
    g_cfg.frameLimit = GetPrivateProfileIntA("Graphics", "FrameLimit", 0, ini);
    g_cfg.interpolation = GetPrivateProfileIntA("Graphics", "FrameInterpolation", 0, ini);
    g_cfg.effectScale = GetPrivateProfileIntA("Graphics", "EffectScale", 1, ini);
    GetPrivateProfileStringA("Graphics", "DepthOfField", "0.0", buf, sizeof buf, ini);
    g_cfg.dofStrength = (float)atof(buf);
    g_cfg.windowWidth = GetPrivateProfileIntA("Window", "Width", 0, ini);
    g_cfg.windowHeight = GetPrivateProfileIntA("Window", "Height", 0, ini);
    g_cfg.fixFullscreen = GetPrivateProfileIntA("Window", "FixFullscreen", 1, ini);
    g_cfg.aimSmoothing = GetPrivateProfileIntA("Control", "AimSmoothing", 0, ini);
    g_cfg.aimPrecision = GetPrivateProfileIntA("Control", "MouseAimPrecision", 1, ini);
    if (g_cfg.aimPrecision < 1) g_cfg.aimPrecision = 1;
    g_cfg.log = GetPrivateProfileIntA("Debug", "Log", 0, ini);
    g_cfg.dumpShaders = GetPrivateProfileIntA("Debug", "DumpShaders", 0, ini);
}

// ---------------------------------------------------------------- 拡大状態
static UINT g_nativeW = 1280, g_nativeH = 720;  // ゲームが要求したバックバッファサイズ
static UINT g_scaledW = 1280, g_scaledH = 720;  // 実際に作るバックバッファサイズ
static float g_scale = 1.0f;
static DWORD g_maxTex = 4096;

// ---------------------------------------------------------------- 拡大したリソースの管理
// 拡大して作成したテクスチャ/サーフェスをポインタで記録する。
// ・SetRenderTarget 時に、どの倍率で座標を拡大すべきかを引く
// ・ゲームがサイズを問い合わせたら(GetLevelDesc / GetDesc)元のサイズを返す。
//   ゲームはテクスチャの大きさから貼り付けサイズを決めるので、拡大後のサイズが見えると絵がズームされる
// サイズで判定すると、拡大後のサイズ(例: 256→512)とゲーム本来の 512x512 テクスチャを取り違えるためポインタで管理する
struct Scaled { UINT nw, nh; float sx, sy; bool deviceOwned; };  // deviceOwned: バックバッファ等。Release が 0 を返してもデバイスが保持している
static std::map<void*, Scaled> g_scaledTex, g_scaledSurf;   // テクスチャ / サーフェス → 元サイズと倍率
static std::map<void*, void*> g_texLevel0;                  // テクスチャ → レベル0 サーフェス

typedef HRESULT(STDMETHODCALLTYPE* PFN_GetLevelDesc)(IDirect3DTexture9*, UINT, D3DSURFACE_DESC*);
typedef HRESULT(STDMETHODCALLTYPE* PFN_GetDesc)(IDirect3DSurface9*, D3DSURFACE_DESC*);
typedef ULONG(STDMETHODCALLTYPE* PFN_Release)(IUnknown*);

// d3d9.dll 内のクラスごとの vtable と、差し替え前の関数
struct VTable { void** vt; PFN_Release release; PFN_GetLevelDesc getLevelDesc; PFN_GetDesc getDesc; };
static std::vector<VTable> g_vtables;

static VTable* vtableOf(void* obj) {
    void** vt = *(void***)obj;
    for (auto& v : g_vtables) if (v.vt == vt) return &v;
    g_vtables.push_back({ vt });
    return &g_vtables.back();
}

static void* patchSlot(void** vt, int index, void* fn) {
    void* orig = vt[index];
    if (orig == fn) return nullptr;
    DWORD old;
    VirtualProtect(&vt[index], sizeof(void*), PAGE_READWRITE, &old);
    vt[index] = fn;
    VirtualProtect(&vt[index], sizeof(void*), old, &old);
    return orig;
}

static void forget(void* obj) {
    auto t = g_texLevel0.find(obj);
    if (t != g_texLevel0.end()) { g_scaledSurf.erase(t->second); g_texLevel0.erase(t); }
    g_scaledTex.erase(obj);
    g_scaledSurf.erase(obj);
}

static ULONG STDMETHODCALLTYPE h_Release(IUnknown* self) {
    PFN_Release orig = vtableOf(self)->release;
    auto sf = g_scaledSurf.find(self);
    bool tracked = g_scaledTex.count(self) || (sf != g_scaledSurf.end() && !sf->second.deviceOwned);
    ULONG r = orig(self);
    if (r == 0 && tracked) forget(self);
    return r;
}

static HRESULT STDMETHODCALLTYPE h_GetLevelDesc(IDirect3DTexture9* self, UINT level, D3DSURFACE_DESC* d) {
    HRESULT hr = vtableOf(self)->getLevelDesc(self, level, d);
    auto it = g_scaledTex.find(self);
    if (SUCCEEDED(hr) && it != g_scaledTex.end()) {
        d->Width = max(1u, it->second.nw >> level);
        d->Height = max(1u, it->second.nh >> level);
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE h_GetDesc(IDirect3DSurface9* self, D3DSURFACE_DESC* d) {
    HRESULT hr = vtableOf(self)->getDesc(self, d);
    auto it = g_scaledSurf.find(self);
    if (SUCCEEDED(hr) && it != g_scaledSurf.end()) { d->Width = it->second.nw; d->Height = it->second.nh; }
    return hr;
}

// 実際のサイズを得る (偽装しない)
static HRESULT realDesc(IDirect3DSurface9* s, D3DSURFACE_DESC* d) {
    VTable* v = vtableOf(s);
    return v->getDesc ? v->getDesc(s, d) : s->GetDesc(d);
}

static void hookSurface(IDirect3DSurface9* s) {
    VTable* v = vtableOf(s);
    if (!v->getDesc) {
        v->release = (PFN_Release)patchSlot(v->vt, 2, (void*)h_Release);
        v->getDesc = (PFN_GetDesc)patchSlot(v->vt, 12, (void*)h_GetDesc);
    }
}

static void trackSurface(IDirect3DSurface9* s, UINT nw, UINT nh) {
    if (!s) return;
    hookSurface(s);
    D3DSURFACE_DESC d;
    if (FAILED(realDesc(s, &d))) return;
    g_scaledSurf[s] = { nw, nh, (float)d.Width / nw, (float)d.Height / nh, false };
}

// バックバッファと自動深度バッファ。ゲームには要求どおりのサイズを見せ、座標は 1280x720 基準の倍率で拡大する
static UINT g_reqW = 1280, g_reqH = 720;
static void trackDeviceSurfaces(IDirect3DDevice9* dev) {
    for (auto it = g_scaledSurf.begin(); it != g_scaledSurf.end();)
        it = it->second.deviceOwned ? g_scaledSurf.erase(it) : std::next(it);
    if (g_scale == 1.0f) return;
    IDirect3DSurface9* s = nullptr;
    if (SUCCEEDED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &s))) {
        hookSurface(s);
        g_scaledSurf[s] = { g_reqW, g_reqH, g_scale, g_scale, true };
        s->Release();
    }
    if (SUCCEEDED(dev->GetDepthStencilSurface(&s))) {
        hookSurface(s);
        g_scaledSurf[s] = { g_reqW, g_reqH, g_scale, g_scale, true };
        s->Release();
    }
}

static void trackTexture(IDirect3DTexture9* t, UINT nw, UINT nh) {
    VTable* v = vtableOf(t);
    if (!v->getLevelDesc) {
        v->release = (PFN_Release)patchSlot(v->vt, 2, (void*)h_Release);
        v->getLevelDesc = (PFN_GetLevelDesc)patchSlot(v->vt, 17, (void*)h_GetLevelDesc);
    }
    g_scaledTex[t] = { nw, nh, 1, 1, false };
    IDirect3DSurface9* s = nullptr;
    if (SUCCEEDED(t->GetSurfaceLevel(0, &s))) {
        trackSurface(s, nw, nh);
        g_texLevel0[t] = s;
        s->Release();
    }
}

// 調査用: 頂点バッファの Lock を数える
typedef HRESULT(STDMETHODCALLTYPE* PFN_VBLock)(IDirect3DVertexBuffer9*, UINT, UINT, void**, DWORD);
static PFN_VBLock o_VBLock;
static UINT g_vbLocks;
static HRESULT STDMETHODCALLTYPE h_VBLock(IDirect3DVertexBuffer9* self, UINT off, UINT size, void** p, DWORD flags) {
    g_vbLocks++;
    return o_VBLock(self, off, size, p, flags);
}

static bool lookupScale(IDirect3DSurface9* s, float* sx, float* sy) {
    *sx = *sy = 1.0f;
    auto it = s ? g_scaledSurf.find(s) : g_scaledSurf.end();
    if (it == g_scaledSurf.end()) return false;
    *sx = it->second.sx; *sy = it->second.sy;
    return true;
}

// レンダーターゲット/深度テクスチャの作成サイズを決める。拡大対象なら true。
static bool scaleRTSize(UINT* w, UINT* h) {
    UINT nw = *w, nh = *h;
    // 画面サイズ、または 1/2, 1/4, 1/8 の縮小バッファ
    for (UINT k = 1; k <= 8; k *= 2) {
        if (nw * k == g_nativeW && nh * k == g_nativeH) {
            if (g_scale == 1.0f) return false;
            *w = (UINT)lroundf(nw * g_scale);
            *h = (UINT)lroundf(nh * g_scale);
            return true;
        }
    }
    // グロー/ブルーム用の縮小バッファ (256, 128, 64, 32 の正方形)
    if (g_cfg.effectScale && nw == nh && (nw == 256 || nw == 128 || nw == 64 || nw == 32) && g_scale > 1.0f) {
        UINT s = (UINT)lroundf(nw * g_scale);
        *w = *h = s;
        return true;
    }
    // 影マップ
    if (nw == 1024 && nh == 1024 && g_cfg.shadowScale > 1) {
        UINT s = 1024 * g_cfg.shadowScale;
        while (s > g_maxTex) s /= 2;
        if (s == 1024) return false;
        *w = *h = s;
        return true;
    }
    return false;
}

static void computeBackBuffer(HWND hwnd, D3DPRESENT_PARAMETERS* p) {
    // ゲームはバックバッファをウィンドウのクライアントサイズで要求するが、
    // 内部のレンダーターゲットと 2D 座標は常に 1280x720 基準なのでそちらを基準にする
    g_nativeW = 1280;
    g_nativeH = 720;
    g_reqW = p->BackBufferWidth ? p->BackBufferWidth : g_nativeW;
    g_reqH = p->BackBufferHeight ? p->BackBufferHeight : g_nativeH;
    UINT tw = g_cfg.renderWidth, th = g_cfg.renderHeight;
    if (!tw || !th) {
        MONITORINFO mi = { sizeof mi };
        GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        tw = mi.rcMonitor.right - mi.rcMonitor.left;
        th = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }
    // アスペクト比はゲームに合わせ、指定サイズに収まる最大の倍率にする
    float s = fminf((float)tw / g_nativeW, (float)th / g_nativeH) * g_cfg.supersample;
    float smax = fminf((float)g_maxTex / g_nativeW, (float)g_maxTex / g_nativeH);
    if (s > smax) s = smax;
    if (s < 1.0f) s = 1.0f;
    g_scale = s;
    g_scaledW = (UINT)lroundf(g_nativeW * s);
    g_scaledH = (UINT)lroundf(g_nativeH * s);
    p->BackBufferWidth = g_scaledW;
    p->BackBufferHeight = g_scaledH;
    // ウィンドウがバックバッファより小さいとき自前で縮小して Present するため、転送元矩形を使える COPY にする
    if (p->Windowed && s != 1.0f) { p->SwapEffect = D3DSWAPEFFECT_COPY; p->BackBufferCount = 1; }
    p->PresentationInterval = g_cfg.frameLimit == 0 ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    L("backbuffer %ux%u -> %ux%u (scale %.3f, target %ux%u)", g_nativeW, g_nativeH, g_scaledW, g_scaledH, s, tw, th);
}

static UINT vertexCount(D3DPRIMITIVETYPE t, UINT n) {
    switch (t) {
    case D3DPT_POINTLIST: return n;
    case D3DPT_LINELIST: return n * 2;
    case D3DPT_LINESTRIP: return n + 1;
    case D3DPT_TRIANGLELIST: return n * 3;
    default: return n + 2;  // strip / fan
    }
}

// ---------------------------------------------------------------- ラッパー
static void scaleRect(RECT* r, float sx, float sy) {
    r->left = lroundf(r->left * sx); r->top = lroundf(r->top * sy);
    r->right = lroundf(r->right * sx); r->bottom = lroundf(r->bottom * sy);
}

static HWND g_hwnd;
static decltype(&SetWindowPos) o_SetWindowPos = SetWindowPos;

static int g_frame;
static bool g_trace;
static int g_drawCount;  // 直前の Present からの描画命令数 (調査用)

static uint32_t g_constHash = 2166136261u, g_prevConstHash;  // フレーム内の頂点シェーダー定数のハッシュ (内容の変化の計測用)

// ---------------------------------------------------------------- 縮小表示
// ウィンドウのクライアント領域がバックバッファより小さいと、D3D9 の Present は最近傍で縮小して
// ギザギザになる。1/2 ずつ線形縮小を重ねてからバックバッファ左上に書き戻し、その範囲だけを Present する。
struct Downscaler {
    struct Tmp { UINT w, h; IDirect3DSurface9* s; };
    std::vector<Tmp> tmps;

    IDirect3DSurface9* get(IDirect3DDevice9* dev, UINT w, UINT h) {
        for (auto& t : tmps) if (t.w == w && t.h == h) return t.s;
        IDirect3DSurface9* s = nullptr;
        if (FAILED(dev->CreateRenderTarget(w, h, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &s, nullptr))) return nullptr;
        tmps.push_back({ w, h, s });
        return s;
    }
    void release() { for (auto& t : tmps) t.s->Release(); tmps.clear(); }

    // 縮小して Present 用の矩形を返す。縮小不要なら false
    bool run(IDirect3DDevice9* dev, IDirect3DSurface9* bb, RECT* src, RECT* dst) {
        RECT rc;
        if (!g_hwnd || !GetClientRect(g_hwnd, &rc) || rc.right <= 0 || rc.bottom <= 0) return false;
        UINT cw = rc.right, ch = rc.bottom;
        if (cw >= g_scaledW && ch >= g_scaledH) return false;
        // アスペクト比を保って収める
        float k = fminf((float)cw / g_scaledW, (float)ch / g_scaledH);
        UINT tw = max(1u, (UINT)lroundf(g_scaledW * k)), th = max(1u, (UINT)lroundf(g_scaledH * k));
        IDirect3DSurface9* cur = bb;
        UINT w = g_scaledW, h = g_scaledH;
        do {
            UINT nw = max(tw, w / 2), nh = max(th, h / 2);
            IDirect3DSurface9* t = get(dev, nw, nh);
            if (!t || FAILED(dev->StretchRect(cur, nullptr, t, nullptr, D3DTEXF_LINEAR))) return false;
            cur = t; w = nw; h = nh;
        } while (w != tw || h != th);
        RECT r = { 0, 0, (LONG)tw, (LONG)th };
        if (FAILED(dev->StretchRect(cur, nullptr, bb, &r, D3DTEXF_POINT))) return false;
        *src = r;
        dst->left = (cw - tw) / 2; dst->top = (ch - th) / 2;
        dst->right = dst->left + tw; dst->bottom = dst->top + th;
        if (tw != cw || th != ch) {
            // 余白を黒で塗る
            HDC dc = GetDC(g_hwnd);
            HRGN all = CreateRectRgn(0, 0, cw, ch), inner = CreateRectRgnIndirect(dst);
            CombineRgn(all, all, inner, RGN_DIFF);
            FillRgn(dc, all, (HBRUSH)GetStockObject(BLACK_BRUSH));
            DeleteObject(all); DeleteObject(inner); ReleaseDC(g_hwnd, dc);
        }
        return true;
    }
} g_down;

// ゲームの全画面化は枠を消して最大化するだけなので、枠の分(-3,-26)ずれて上端が欠ける。
// 枠なし状態ならモニター矩形にぴったり合わせる
static void fixFullscreenWindow() {
    if (!g_cfg.fixFullscreen || !g_hwnd || (GetWindowLongA(g_hwnd, GWL_STYLE) & WS_CAPTION)) return;
    MONITORINFO mi = { sizeof mi };
    GetMonitorInfoA(MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST), &mi);
    RECT r;
    GetWindowRect(g_hwnd, &r);
    if (!EqualRect(&r, &mi.rcMonitor)) {
        L("fix fullscreen window %ld,%ld,%ld,%ld", r.left, r.top, r.right, r.bottom);
        o_SetWindowPos(g_hwnd, nullptr, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                       mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// 頂点シェーダー定数をデバイスへ送る (ログ有効時は内容の変化の計測用にハッシュを取る)
static HRESULT sendVSConst(IDirect3DDevice9* dev, UINT r, const float* d, UINT n) {
    if (g_cfg.log) {
        const BYTE* b = (const BYTE*)d;
        for (UINT i = 0; i < n * 16; i++) g_constHash = (g_constHash ^ b[i]) * 16777619u;
    }
    return dev->SetVertexShaderConstantF(r, d, n);
}

// ---------------------------------------------------------------- 時刻 / フレームレート上限
static double nowMs() {
    static LARGE_INTEGER freq = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return t.QuadPart * 1000.0 / freq.QuadPart;
}

// 上限 fps に合わせて Present の間隔を揃える。大半は高精度タイマーで眠り、最後の少しだけ回して待つ
static void limitFrameRate() {
    if (g_cfg.frameLimit <= 0) return;
    static HANDLE timer = CreateWaitableTimerExW(NULL, NULL, 0x2 /* CREATE_WAITABLE_TIMER_HIGH_RESOLUTION */, TIMER_ALL_ACCESS);
    static double next = 0;
    double period = 1000.0 / g_cfg.frameLimit, now = nowMs();
    if (next - now > 1.5 && timer) {
        LARGE_INTEGER due; due.QuadPart = -(LONGLONG)((next - now - 1.0) * 10000);
        SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE);
        WaitForSingleObject(timer, 100);
    }
    while ((now = nowMs()) < next) YieldProcessor();
    next = (now - next > period) ? now + period : next + period;  // 大きく遅れたら基準を取り直す
}

// ---------------------------------------------------------------- フレーム補間
// ゲームロジックは 60Hz で、描画は同じ状態を何度も描き直している。頂点シェーダー定数 (カメラ・物体・ボーンの行列など) を
// 描画ごとに記録し、値が変わったら「直前の値」から「最新の値」へ、変わった時刻からの経過時間に応じて線形補間して送る。
// ・ロジックと描画は別スレッドなので、1回の更新が描画の途中で分かれて届く (物体が先・カメラが後など)。
//   そのため画面全体ではなく描画ごとに、それぞれが変わった時刻を起点にする。
// ・描画は シェーダー/頂点バッファ/テクスチャ/範囲 の組 + 同じ組の出現順 で識別する。
//   同じ形の物体が複数あると、画面外に出入りしたり並べ替えられたりして順番がずれ、別の物体と取り違える。
//   取り違えは「前回の移動量から想定できない変化」「前のフレームより増えた分」で見分け、補間せずに履歴を作り直す。
// 表示は 1 更新 (約 16ms) 遅れる。
struct Interp {
    struct Upload { UINT reg, count, off; };  // off は data 内の位置 (float 単位)
    // 描画ごとの履歴: 定数の並び (レジスタと個数)、直前の値・最新の値、最新の値になった時刻
    struct Hist {
        std::vector<Upload> layout;
        std::vector<float> prev, cur;
        double changed = -1e9, seen = 0;
        bool moving = false;  // prev→cur が実際の動き (連続性の判定に使える) か
    };
    std::unordered_map<uint64_t, Hist> hist;
    std::unordered_map<uint64_t, UINT> seenSig, lastCount;  // 組ごとの出現回数 (このフレーム / 直前のフレーム)
    std::vector<Upload> ups;                                // まだ描画に使われていない定数
    std::vector<float> data;
    std::vector<float> tmp;
    double frameTime = 0, lastChange = -1e9, interval = 1000.0 / 60.6, alpha = 1;
    UINT frames = 0;
    CRITICAL_SECTION cs;
    struct Stats { UINT states, draws, matched, ups, lerped, changed, flushed, unmatchedChanged; } st = {};
    Interp() { InitializeCriticalSection(&cs); }

    void upload(UINT reg, const float* d, UINT n) {
        EnterCriticalSection(&cs);
        ups.push_back({ reg, n, (UINT)data.size() });
        data.insert(data.end(), d, d + n * 4);
        LeaveCriticalSection(&cs);
    }

    // 前後の値がかけ離れていたら (瞬間移動など) 補間しない
    static bool similar(const float* a, const float* b, UINT n) {
        for (UINT k = 0; k < n; k++) {
            float d = fabsf(b[k] - a[k]), m = fmaxf(fabsf(a[k]), fabsf(b[k]));
            if (d > 0.25f * (m + 0.25f)) return false;
        }
        return true;
    }

    // 今回の変化 (cur→next) が前回の動き (prev→cur) から続くものか。別の物体に入れ替わると大きく外れる。
    // 物体全体の位置を表す小さな行列 (4 レジスタ以下) だけで判定する。
    // キャラの骨の配列は攻撃や移動の開始で動きが急に変わるのが普通なので判定に使わない
    bool continuous(const Hist& h) const {
        for (const Upload& u : ups) {
            if (u.count > 4) continue;
            const float *p = &h.prev[u.off], *c = &h.cur[u.off], *n = &data[u.off];
            for (UINT k = 0; k < u.count * 4; k++)
                if (fabsf(n[k] - c[k]) > 2.5f * fabsf(c[k] - p[k]) + 0.02f * (fabsf(c[k]) + 0.05f)) return false;
        }
        return true;
    }

    static void reset(Hist& h, const std::vector<float>& d) {
        h.prev = h.cur = d;
        h.changed = -1e9;
        h.moving = false;
    }

    // 描画の直前: この描画のための定数を (必要なら補間して) 送る
    void draw(IDirect3DDevice9* dev, uint64_t sig) {
        EnterCriticalSection(&cs);
        UINT occ = seenSig[sig]++;
        uint64_t key = sig ^ (occ * 0x9E3779B97F4A7C15ull);
        Hist& h = hist[key];
        h.seen = frameTime;
        st.draws++;
        bool sameLayout = h.layout.size() == ups.size();
        for (size_t j = 0; sameLayout && j < ups.size(); j++)
            sameLayout = h.layout[j].reg == ups[j].reg && h.layout[j].count == ups[j].count;
        auto lc = lastCount.find(sig);
        bool extra = lc == lastCount.end() || occ >= lc->second;  // 直前のフレームより増えた分
        if (!sameLayout) {
            // 初めての描画、または定数の並びが変わった
            h.layout = ups;
            reset(h, data);
            st.unmatchedChanged++;
        } else if (!data.empty() && memcmp(h.cur.data(), data.data(), data.size() * 4)) {
            if (extra || (h.moving && !continuous(h))) {
                // 別の物体と入れ替わった
                reset(h, data);
                st.unmatchedChanged++;
            } else {
                // ロジックが進んで値が変わった
                h.prev.swap(h.cur);
                h.cur = data;
                h.changed = frameTime;
                h.moving = true;
                st.changed++;
                if (frameTime - lastChange > interval * 0.6) { lastChange = frameTime; st.states++; }
            }
            st.matched++;
        } else {
            st.matched++;
        }
        // 補間するかは描画単位でまとめて決める (骨ごとに分かれると体のパーツがずれる)。
        // 補間に向かない値が1つでもあれば、描画全体を直前/最新のうち時刻が近い方で描き、遅れ方を他の描画と揃える
        float a = (float)((frameTime - h.changed) / interval);
        bool lerp = a < 1.0f;
        for (size_t j = 0; lerp && j < ups.size(); j++)
            lerp = similar(&h.prev[ups[j].off], &h.cur[ups[j].off], ups[j].count * 4);
        const std::vector<float>& snap = (a < 0.5f) ? h.prev : h.cur;
        for (const Upload& u : ups) {
            const float* src = &data[u.off];
            if (lerp) {
                const float* p = &h.prev[u.off];
                const float* c = &h.cur[u.off];
                tmp.resize(u.count * 4);
                for (UINT k = 0; k < u.count * 4; k++) tmp[k] = p[k] + (c[k] - p[k]) * a;
                src = tmp.data();
                st.lerped++;
            } else if (a < 1.0f) {
                src = &snap[u.off];
            }
            sendVSConst(dev, u.reg, src, u.count);
            st.ups++;
        }
        ups.clear();
        data.clear();
        LeaveCriticalSection(&cs);
    }

    // 描画を伴わない定数 (フレームの終わりなど) はそのまま送る
    void flush(IDirect3DDevice9* dev) {
        EnterCriticalSection(&cs);
        for (const Upload& u : ups) {
            sendVSConst(dev, u.reg, &data[u.off], u.count);
            st.flushed++;
        }
        ups.clear();
        data.clear();
        LeaveCriticalSection(&cs);
    }

    void endFrame(IDirect3DDevice9* dev) {
        flush(dev);
        EnterCriticalSection(&cs);
        lastCount.swap(seenSig);
        seenSig.clear();
        frameTime = nowMs();
        alpha = fmin(1.0, (frameTime - lastChange) / interval);
        // しばらく描かれていない描画の履歴を捨てる
        if (++frames % 600 == 0) {
            for (auto it = hist.begin(); it != hist.end();)
                it = (frameTime - it->second.seen > 3000) ? hist.erase(it) : std::next(it);
        }
        LeaveCriticalSection(&cs);
    }
} g_interp;

// 調査用: [Debug] CaptureAt=フレーム番号 から CaptureFrames 枚、バックバッファを 320x180 に縮小して capture\NNN.raw (BGRA) に保存
static void captureFrame(IDirect3DDevice9* dev) {
    static int frame = 0, at = -1, count = 0;
    static IDirect3DSurface9 *shrunk = nullptr, *sys = nullptr;
    static std::string dir = std::string(g_dir) + "capture";
    if (at < 0) {
        std::string ini = std::string(g_dir) + "d3d9mod.ini";
        at = GetPrivateProfileIntA("Debug", "CaptureAt", 0, ini.c_str());
        count = GetPrivateProfileIntA("Debug", "CaptureFrames", 0, ini.c_str());
    }
    frame++;
    // F12 でその直後の 24 フレームを保存する (2 回目以降は capture2, capture3 ... へ)
    if ((GetAsyncKeyState(VK_F12) & 1) && frame >= at + count) {
        static int shot = 0;
        at = frame; count = 24;
        dir = std::string(g_dir) + "capture" + (++shot > 1 ? std::to_string(shot) : "");
        L("F12 capture -> %s", dir.c_str());
    }
    if (!count || frame < at || frame >= at + count) return;
    if (frame == at) CreateDirectoryA(dir.c_str(), NULL);
    if (!shrunk) {
        dev->CreateRenderTarget(320, 180, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &shrunk, nullptr);
        dev->CreateOffscreenPlainSurface(320, 180, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &sys, nullptr);
    }
    IDirect3DSurface9* bb = nullptr;
    if (!shrunk || !sys || FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb))) return;
    dev->StretchRect(bb, nullptr, shrunk, nullptr, D3DTEXF_LINEAR);
    bb->Release();
    D3DLOCKED_RECT lr;
    if (SUCCEEDED(dev->GetRenderTargetData(shrunk, sys)) && SUCCEEDED(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY))) {
        char path[MAX_PATH];
        sprintf(path, "%s\\%03d.raw", dir.c_str(), frame - at);
        if (FILE* f = fopen(path, "wb")) {
            for (int y = 0; y < 180; y++) fwrite((BYTE*)lr.pBits + y * lr.Pitch, 4, 320, f);
            fclose(f);
        }
        sys->UnlockRect();
        L("captured frame %d at %.2fms draws %d alpha %.2f states %u changedUploads %u flushed %u unmatched %u locks %u",
          frame - at, nowMs(), g_drawCount, g_interp.alpha, g_interp.st.states, g_interp.st.changed, g_interp.st.flushed,
          g_interp.st.unmatchedChanged, g_vbLocks);
    }
}

// ---------------------------------------------------------------- マウス照準の細かさ
// 照準はマウスの移動量 (カウント) × input.rotateRatioMouse (スクリプトが 0.087 を設定) × rotateRatio × 感度 で回る。
// 初期設定で 1 カウント約 0.23 度と粗いので、rotateRatioMouse を 1/N にする。スクリプトが値を設定し直したら掛け直す。
static void applyMouseAimPrecision() {
    if (g_cfg.aimPrecision <= 1) return;
    static float written = 0;
    BYTE* base = (BYTE*)GetModuleHandleA(NULL);
    BYTE** pInput = (BYTE**)(base + (0x61FA8C - 0x400000));  // input オブジェクトへのポインタ
    BYTE* input = *pInput;
    if (!input || IsBadWritePtr(input + 0x1A4, 4)) return;
    float* ratio = (float*)(input + 0x1A4);                     // rotateRatioMouse
    if (*ratio > 0 && *ratio != written) {
        float v = *ratio / g_cfg.aimPrecision;
        L("rotateRatioMouse %g -> %g", *ratio, v);
        *ratio = written = v;
    }
}

static HRESULT presentWith(IDirect3DDevice9* dev, const RECT* src, const RECT* dst, HWND w, const RGNDATA* rgn,
                           HRESULT (*fn)(void*, const RECT*, const RECT*, HWND, const RGNDATA*), void* ctx) {
    applyMouseAimPrecision();
    fixFullscreenWindow();
    if (g_cfg.log) {
        // 5 秒ごとに実測フレームレートを記録
        static DWORD t0 = GetTickCount(), n = 0, changed = 0;
        n++;
        if (g_constHash != g_prevConstHash) changed++;
        g_prevConstHash = g_constHash; g_constHash = 2166136261u;
        DWORD now = GetTickCount();
        if (now - t0 >= 5000) {
            L("fps %.1f (content changes %.1f/s)", n * 1000.0 / (now - t0), changed * 1000.0 / (now - t0));
            auto& st = g_interp.st;
            if (g_cfg.interpolation)
                L("  interp: states %u, draws %u matched %u, uploads %u lerped %u (changed %u), interval %.2fms",
                  st.states, st.draws, st.matched, st.ups, st.lerped, st.changed, g_interp.interval);
            st = {};
            t0 = now; n = 0; changed = 0;
        }
    }
    if (g_cfg.dumpShaders) {
        if (g_trace) L("=== end trace");
        g_trace = (++g_frame % 1800) == 0;
        if (g_trace) L("=== trace frame %d", g_frame);
    }
    if (g_cfg.interpolation) g_interp.flush(dev);
    if (g_cfg.log) captureFrame(dev);
    g_drawCount = 0;
    RECT s, d;
    bool down = false;
    if (!src && !dst && !w && g_scale != 1.0f) {
        IDirect3DSurface9* bb = nullptr;
        if (SUCCEEDED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb))) {
            down = g_down.run(dev, bb, &s, &d);
            bb->Release();
        }
    }
    limitFrameRate();
    HRESULT hr = down ? fn(ctx, &s, &d, w, rgn) : fn(ctx, src, dst, w, rgn);
    if (g_cfg.interpolation) g_interp.endFrame(dev);
    return hr;
}

// ---------------------------------------------------------------- シェーダー (調査用ダンプ)
// バイトコードのハッシュでシェーダーを識別する (実行ごとにポインタは変わるが内容は同じ)
static uint32_t hashCode(const DWORD* code) {
    uint32_t h = 2166136261u;
    for (const DWORD* p = code; ; p++) {
        h = (h ^ *p) * 16777619u;
        if (*p == 0x0000FFFF) break;  // END トークン
    }
    return h;
}
static std::map<void*, uint32_t> g_shaderHash;

static size_t codeLength(const DWORD* code) {
    size_t n = 0;
    while (code[n] != 0x0000FFFF) n++;
    return n + 1;
}

// def cN, a, b, c, d 命令の定数を探す (ps_2_0 の def はトークン 0x05000051 + レジスタ + float4)
static float* findDef(std::vector<DWORD>& code, int reg) {
    DWORD dst = 0xA00F0000 | reg;  // D3DSPR_CONST, 全成分書き込み
    for (size_t i = 1; i + 5 < code.size(); i++)
        if ((code[i] & 0xFFFF) == D3DSIO_DEF && code[i + 1] == dst) return (float*)&code[i + 2];
    return nullptr;
}

// 最終合成シェーダー (ps_aca4efd9): 深度に応じて画面を6点平均でぼかす被写界深度。
// ぼかし半径 = 深度 * c2.x + c2.y (720p 基準のピクセル数)。この2つに強さを掛ける
static const uint32_t PS_FINAL_COMPOSITE = 0xaca4efd9;
static bool patchFinalComposite(std::vector<DWORD>& code, float k) {
    float* c2 = findDef(code, 2);
    if (!c2 || c2[0] != 3.0f || c2[1] != 0.5f) return false;
    c2[0] *= k;
    c2[1] *= k;
    return true;
}

struct ID3DXBufferMin : IUnknown {
    virtual LPVOID STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0;
};
static void dumpShader(const char* kind, uint32_t hash, const DWORD* code) {
    typedef HRESULT(WINAPI* PFN_Disasm)(const DWORD*, BOOL, LPCSTR, ID3DXBufferMin**);
    static PFN_Disasm disasm = (PFN_Disasm)GetProcAddress(LoadLibraryA("d3dx9_43.dll"), "D3DXDisassembleShader");
    char path[MAX_PATH];
    sprintf(path, "%sshaderdump", g_dir);
    CreateDirectoryA(path, NULL);
    sprintf(path, "%sshaderdump\\%s_%08x.txt", g_dir, kind, hash);
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return;
    ID3DXBufferMin* buf = nullptr;
    if (!disasm || FAILED(disasm(code, FALSE, NULL, &buf))) return;
    if (FILE* f = fopen(path, "wb")) { fwrite(buf->GetBufferPointer(), 1, strlen((char*)buf->GetBufferPointer()), f); fclose(f); }
    buf->Release();
}

struct SwapChain : SwapChainBase {
    IDirect3DDevice9* dev;
    SwapChain(IDirect3DSwapChain9* r, IDirect3DDevice9* d) : SwapChainBase(r), dev(d) {}
    STDMETHOD(Present)(THIS_ CONST RECT* a, CONST RECT* b, HWND w, CONST RGNDATA* c, DWORD f) override {
        struct Ctx { IDirect3DSwapChain9* sc; DWORD f; } ctx = { real, f };
        return presentWith(dev, a, b, w, c, [](void* p, const RECT* a, const RECT* b, HWND w, const RGNDATA* c) {
            auto x = (Ctx*)p; return x->sc->Present(a, b, w, c, x->f); }, &ctx);
    }
};

struct Dev : DeviceBase {
    float sx = 1, sy = 1;          // 現在の RT0 の倍率
    bool fvfMode = true;           // SetFVF と SetVertexDeclaration のどちらが最後か
    bool rhw = false;              // 現在の頂点形式が画面座標(XYZRHW / POSITIONT)か
    int rhwOffset = 0;             // 頂点内の位置オフセット
    std::vector<BYTE> tmp;
    uint32_t curPS = 0;            // 現在のピクセルシェーダーのハッシュ
    void *vs = nullptr, *ps = nullptr, *vb0 = nullptr, *ib = nullptr;  // フレーム補間で描画を対応付けるための現在の状態
    IDirect3DBaseTexture9* tex[8] = {};
    UINT rtW = 0, rtH = 0;         // 現在の RT0 の (ゲームから見た) サイズ

    Dev(IDirect3DDevice9* r) : DeviceBase(r) {}

    void traceDraw(const char* kind, UINT n) {
        char t[256] = "", *q = t;
        for (int i = 0; i < 8; i++) {
            if (!tex[i] || tex[i]->GetType() != D3DRTYPE_TEXTURE) continue;
            D3DSURFACE_DESC d;
            ((IDirect3DTexture9*)tex[i])->GetLevelDesc(0, &d);
            q += sprintf(q, " t%d=%ux%u%s", i, d.Width, d.Height, (d.Usage & D3DUSAGE_RENDERTARGET) ? "rt" : "");
        }
        L("  %s n=%u rt=%ux%u ps=%08x%s", kind, n, rtW, rtH, curPS, t);
    }

    STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* code, IDirect3DPixelShader9** out) override {
        uint32_t h = hashCode(code);
        HRESULT hr = E_FAIL;
        if (h == PS_FINAL_COMPOSITE && g_cfg.dofStrength != 1.0f) {
            std::vector<DWORD> mod(code, code + codeLength(code));
            if (patchFinalComposite(mod, g_cfg.dofStrength)) {
                hr = real->CreatePixelShader(mod.data(), out);
                L("depth of field strength %.2f: %s", g_cfg.dofStrength, SUCCEEDED(hr) ? "patched" : "failed");
            }
        }
        if (FAILED(hr)) hr = real->CreatePixelShader(code, out);
        if (SUCCEEDED(hr) && g_cfg.dumpShaders) {
            g_shaderHash[*out] = h;
            dumpShader("ps", h, code);
        }
        return hr;
    }
    STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* s) override { vs = s; return real->SetVertexShader(s); }
    STDMETHOD(CreateVertexBuffer)(THIS_ UINT len, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9** out, HANDLE* sh) override {
        HRESULT hr = real->CreateVertexBuffer(len, usage, fvf, pool, out, sh);
        if (SUCCEEDED(hr) && g_cfg.log) {
            void** vt = *(void***)*out;
            if (vt[11] != (void*)h_VBLock) { void* o = patchSlot(vt, 11, (void*)h_VBLock); if (o) o_VBLock = (PFN_VBLock)o; }
        }
        return hr;
    }
    STDMETHOD(SetStreamSource)(THIS_ UINT n, IDirect3DVertexBuffer9* b, UINT off, UINT stride) override {
        if (n == 0) vb0 = b;
        return real->SetStreamSource(n, b, off, stride);
    }
    STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* i) override { ib = i; return real->SetIndices(i); }

    // 補間が有効なら、描画の直前に定数を送る
    void interpDraw(uint64_t a, uint64_t b) {
        g_drawCount++;
        if (!g_cfg.interpolation) return;
        uint64_t h = 1469598103934665603ull;
        for (uint64_t v : { (uint64_t)vs, (uint64_t)ps, (uint64_t)vb0, (uint64_t)ib, (uint64_t)tex[0], a, b })
            h = (h ^ v) * 1099511628211ull;
        g_interp.draw(real, h);
    }
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE t, CONST D3DMATRIX* m) override {
        if (g_cfg.log) { static int n; if (n++ % 2000 == 0) L("SetTransform %d (count %d)", t, n); }
        return real->SetTransform(t, m);
    }
    STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT r, float* d, UINT n) override {
        if (g_cfg.interpolation) g_interp.flush(real);
        return real->GetVertexShaderConstantF(r, d, n);
    }
    STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* ps) override {
        this->ps = ps;
        if (g_cfg.dumpShaders) { auto it = g_shaderHash.find(ps); curPS = it != g_shaderHash.end() ? it->second : 0; }
        return real->SetPixelShader(ps);
    }
    STDMETHOD(SetTexture)(THIS_ DWORD st, IDirect3DBaseTexture9* t) override {
        if (st < 8) tex[st] = t;
        return real->SetTexture(st, t);
    }

    bool scaled() const { return sx != 1.0f || sy != 1.0f; }

    // 画面座標頂点を拡大したコピーを作る (ピクセル中心の -0.5 オフセットを保つ)
    const void* scaleVerts(const void* src, UINT count, UINT stride) {
        tmp.resize((size_t)count * stride);
        memcpy(tmp.data(), src, tmp.size());
        for (UINT i = 0; i < count; i++) {
            float* p = (float*)(tmp.data() + (size_t)i * stride + rhwOffset);
            p[0] = (p[0] + 0.5f) * sx - 0.5f;
            p[1] = (p[1] + 0.5f) * sy - 0.5f;
        }
        return tmp.data();
    }

    STDMETHOD(Present)(THIS_ CONST RECT* a, CONST RECT* b, HWND w, CONST RGNDATA* c) override {
        return presentWith(real, a, b, w, c, [](void* p, const RECT* a, const RECT* b, HWND w, const RGNDATA* c) {
            return ((IDirect3DDevice9*)p)->Present(a, b, w, c); }, real);
    }
    STDMETHOD(GetSwapChain)(THIS_ UINT i, IDirect3DSwapChain9** o) override {
        HRESULT hr = real->GetSwapChain(i, o);
        if (SUCCEEDED(hr) && i == 0) *o = new SwapChain(*o, real);
        return hr;
    }
    STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* p) override {
        g_down.release();
        D3DPRESENT_PARAMETERS pp = *p;
        computeBackBuffer(pp.hDeviceWindow, &pp);
        HRESULT hr = real->Reset(&pp);
        L("Reset -> 0x%lx", hr);
        if (SUCCEEDED(hr)) trackDeviceSurfaces(real);
        sx = sy = (g_scale != 1.0f) ? g_scale : 1.0f;
        return hr;
    }

    STDMETHOD(SetRenderTarget)(THIS_ DWORD i, IDirect3DSurface9* s) override {
        HRESULT hr = real->SetRenderTarget(i, s);
        if (i == 0 && SUCCEEDED(hr)) {
            lookupScale(s, &sx, &sy);
            if (g_trace) { D3DSURFACE_DESC d = {}; if (s) s->GetDesc(&d); rtW = d.Width; rtH = d.Height; L("SetRenderTarget %ux%u", rtW, rtH); }
        } else if (g_trace && SUCCEEDED(hr)) L("SetRenderTarget[%lu] %p", i, s);
        return hr;
    }
    STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* v) override {
        if (!scaled()) return real->SetViewport(v);
        D3DVIEWPORT9 vp = *v;
        vp.X = lroundf(v->X * sx); vp.Y = lroundf(v->Y * sy);
        vp.Width = lroundf(v->Width * sx); vp.Height = lroundf(v->Height * sy);
        return real->SetViewport(&vp);
    }
    STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* v) override {
        HRESULT hr = real->GetViewport(v);
        if (SUCCEEDED(hr) && scaled()) {
            v->X = lroundf(v->X / sx); v->Y = lroundf(v->Y / sy);
            v->Width = lroundf(v->Width / sx); v->Height = lroundf(v->Height / sy);
        }
        return hr;
    }
    STDMETHOD(SetScissorRect)(THIS_ CONST RECT* r) override {
        if (!scaled()) return real->SetScissorRect(r);
        RECT rc = *r; scaleRect(&rc, sx, sy);
        return real->SetScissorRect(&rc);
    }
    STDMETHOD(GetScissorRect)(THIS_ RECT* r) override {
        HRESULT hr = real->GetScissorRect(r);
        if (SUCCEEDED(hr) && scaled()) scaleRect(r, 1 / sx, 1 / sy);
        return hr;
    }
    STDMETHOD(Clear)(THIS_ DWORD n, CONST D3DRECT* r, DWORD f, D3DCOLOR c, float z, DWORD st) override {
        if (!n || !r || !scaled()) return real->Clear(n, r, f, c, z, st);
        std::vector<D3DRECT> rc(r, r + n);
        for (auto& x : rc) { x.x1 = lroundf(x.x1 * sx); x.y1 = lroundf(x.y1 * sy); x.x2 = lroundf(x.x2 * sx); x.y2 = lroundf(x.y2 * sy); }
        return real->Clear(n, rc.data(), f, c, z, st);
    }
    STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* a, CONST RECT* ra, IDirect3DSurface9* b, CONST RECT* rb, D3DTEXTUREFILTERTYPE f) override {
        RECT r1, r2; float ax, ay, bx, by;
        if (ra && lookupScale(a, &ax, &ay)) { r1 = *ra; scaleRect(&r1, ax, ay); ra = &r1; }
        if (rb && lookupScale(b, &bx, &by)) { r2 = *rb; scaleRect(&r2, bx, by); rb = &r2; }
        return real->StretchRect(a, ra, b, rb, f);
    }
    // 拡大済み RT から等倍のシステムメモリへ読み戻す場合は、縮小コピーを経由する
    STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* src, IDirect3DSurface9* dst) override {
        D3DSURFACE_DESC ds, dd;
        if (SUCCEEDED(realDesc(src, &ds)) && SUCCEEDED(realDesc(dst, &dd)) && (ds.Width != dd.Width || ds.Height != dd.Height)) {
            IDirect3DSurface9* t = nullptr;
            HRESULT hr = real->CreateRenderTarget(dd.Width, dd.Height, ds.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &t, nullptr);
            if (SUCCEEDED(hr)) {
                real->StretchRect(src, nullptr, t, nullptr, D3DTEXF_LINEAR);
                hr = real->GetRenderTargetData(t, dst);
                t->Release();
            }
            return hr;
        }
        return real->GetRenderTargetData(src, dst);
    }

    STDMETHOD(CreateTexture)(THIS_ UINT w, UINT h, UINT lv, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DTexture9** t, HANDLE* sh) override {
        UINT ow = w, oh = h;
        bool scaled = (u & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) && scaleRTSize(&w, &h);
        HRESULT hr = real->CreateTexture(w, h, lv, u, f, p, t, sh);
        if (scaled && SUCCEEDED(hr)) {
            L("RT texture %ux%u -> %ux%u (fmt %d)", ow, oh, w, h, f);
            trackTexture(*t, ow, oh);
        }
        return hr;
    }
    STDMETHOD(CreateRenderTarget)(THIS_ UINT w, UINT h, D3DFORMAT f, D3DMULTISAMPLE_TYPE m, DWORD q, BOOL l, IDirect3DSurface9** s, HANDLE* sh) override {
        UINT ow = w, oh = h;
        bool scaled = scaleRTSize(&w, &h);
        HRESULT hr = real->CreateRenderTarget(w, h, f, m, q, l, s, sh);
        if (scaled && SUCCEEDED(hr)) { L("RT surface %ux%u -> %ux%u", ow, oh, w, h); trackSurface(*s, ow, oh); }
        return hr;
    }
    STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT w, UINT h, D3DFORMAT f, D3DMULTISAMPLE_TYPE m, DWORD q, BOOL d, IDirect3DSurface9** s, HANDLE* sh) override {
        UINT ow = w, oh = h;
        bool scaled = scaleRTSize(&w, &h);
        HRESULT hr = real->CreateDepthStencilSurface(w, h, f, m, q, d, s, sh);
        if (scaled && SUCCEEDED(hr)) { L("DS surface %ux%u -> %ux%u", ow, oh, w, h); trackSurface(*s, ow, oh); }
        return hr;
    }

    STDMETHOD(SetFVF)(THIS_ DWORD f) override {
        fvfMode = true;
        rhw = (f & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW;
        rhwOffset = 0;
        return real->SetFVF(f);
    }
    STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* d) override {
        fvfMode = false; rhw = false;
        D3DVERTEXELEMENT9 el[MAXD3DDECLLENGTH + 1]; UINT n = MAXD3DDECLLENGTH + 1;
        if (d && SUCCEEDED(d->GetDeclaration(el, &n))) {
            for (UINT i = 0; i < n && el[i].Stream != 0xFF; i++)
                if (el[i].Stream == 0 && el[i].Usage == D3DDECLUSAGE_POSITIONT) { rhw = true; rhwOffset = el[i].Offset; }
        }
        return real->SetVertexDeclaration(d);
    }
    STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE t, UINT c, CONST void* d, UINT s) override {
        if (g_trace) traceDraw("DPUP", c);
        interpDraw(((uint64_t)t << 32) | c, s);
        if (rhw && scaled()) d = scaleVerts(d, vertexCount(t, c), s);
        return real->DrawPrimitiveUP(t, c, d, s);
    }
    STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE t, UINT minIdx, UINT nv, UINT c, CONST void* idx, D3DFORMAT f, CONST void* d, UINT s) override {
        interpDraw(((uint64_t)t << 32) | c, ((uint64_t)nv << 32) | s);
        if (rhw && scaled()) {
            // 頂点は minIdx から nv 個が参照される。先頭からまとめてコピーする
            const BYTE* base = (const BYTE*)d;
            scaleVerts(base, minIdx + nv, s);
            d = tmp.data();
        }
        return real->DrawIndexedPrimitiveUP(t, minIdx, nv, c, idx, f, d, s);
    }
    STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE t, UINT a, UINT b) override {
        if (g_trace) traceDraw("DP", b);
        interpDraw(((uint64_t)t << 32) | a, b);
        if (rhw && scaled()) warnVB();
        return real->DrawPrimitive(t, a, b);
    }
    STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE t, INT a, UINT b, UINT c, UINT d, UINT e) override {
        if (g_trace) traceDraw("DIP", e);
        interpDraw(((uint64_t)a << 32) | b, ((uint64_t)d << 32) | e);
        if (rhw && scaled()) warnVB();
        return real->DrawIndexedPrimitive(t, a, b, c, d, e);
    }
    void warnVB() { static bool once; if (!once) { once = true; L("warning: pre-transformed draw from vertex buffer (not scaled)"); } }

    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT r, CONST float* d, UINT n) override {
        if (g_cfg.interpolation) { g_interp.upload(r, d, n); return D3D_OK; }
        return sendVSConst(real, r, d, n);
    }
    STDMETHOD(SetSamplerState)(THIS_ DWORD st, D3DSAMPLERSTATETYPE t, DWORD v) override {
        if (g_cfg.anisotropy > 1) {
            if (t == D3DSAMP_MINFILTER && v == D3DTEXF_LINEAR) {
                real->SetSamplerState(st, D3DSAMP_MAXANISOTROPY, g_cfg.anisotropy);
                v = D3DTEXF_ANISOTROPIC;
            } else if (t == D3DSAMP_MAXANISOTROPY) {
                v = g_cfg.anisotropy;
            }
        }
        return real->SetSamplerState(st, t, v);
    }
};

static void applyWindowSize();

struct D3D : D3D9Base {
    using D3D9Base::D3D9Base;
    STDMETHOD(CreateDevice)(THIS_ UINT ad, D3DDEVTYPE t, HWND hw, DWORD bf, D3DPRESENT_PARAMETERS* p, IDirect3DDevice9** out) override {
        D3DCAPS9 caps;
        if (SUCCEEDED(real->GetDeviceCaps(ad, t, &caps))) {
            g_maxTex = min(caps.MaxTextureWidth, caps.MaxTextureHeight);
            if ((DWORD)g_cfg.anisotropy > caps.MaxAnisotropy) g_cfg.anisotropy = caps.MaxAnisotropy;
        }
        D3DPRESENT_PARAMETERS pp = *p;
        g_hwnd = p->hDeviceWindow ? p->hDeviceWindow : hw;
        computeBackBuffer(g_hwnd, &pp);
        HRESULT hr = real->CreateDevice(ad, t, hw, bf, &pp, out);
        L("CreateDevice -> 0x%lx", hr);
        if (FAILED(hr) && g_scale != 1.0f) {
            // 失敗したら等倍で作り直す
            L("falling back to native resolution");
            pp = *p; g_scale = 1.0f;
            hr = real->CreateDevice(ad, t, hw, bf, &pp, out);
        }
        if (SUCCEEDED(hr)) {
            trackDeviceSurfaces(*out);
            Dev* d = new Dev(*out);
            d->sx = d->sy = g_scale;
            *out = d;
            applyWindowSize();
        }
        return hr;
    }
};

// ---------------------------------------------------------------- ウィンドウ API フック (IAT)
static void* hookIAT(const char* dll, const char* func, void* repl) {
    BYTE* base = (BYTE*)GetModuleHandleA(NULL);
    auto dos = (IMAGE_DOS_HEADER*)base;
    auto nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    for (auto imp = (IMAGE_IMPORT_DESCRIPTOR*)(base + dir.VirtualAddress); imp->Name; imp++) {
        if (_stricmp((char*)(base + imp->Name), dll)) continue;
        auto names = (IMAGE_THUNK_DATA*)(base + imp->OriginalFirstThunk);
        auto addrs = (IMAGE_THUNK_DATA*)(base + imp->FirstThunk);
        for (; names->u1.AddressOfData; names++, addrs++) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
            auto ibn = (IMAGE_IMPORT_BY_NAME*)(base + names->u1.AddressOfData);
            if (strcmp((char*)ibn->Name, func)) continue;
            DWORD old;
            VirtualProtect(&addrs->u1.Function, sizeof(void*), PAGE_READWRITE, &old);
            void* orig = (void*)addrs->u1.Function;
            addrs->u1.Function = (DWORD_PTR)repl;
            VirtualProtect(&addrs->u1.Function, sizeof(void*), old, &old);
            return orig;
        }
    }
    return nullptr;
}

// ゲームの既定ウィンドウ(クライアント 1280x720)を、設定したクライアントサイズへ置き換える
static HWND g_gameWnd;

static void frameSize(DWORD style, DWORD ex, int cw, int ch, int* w, int* h) {
    RECT r = { 0, 0, cw, ch };
    AdjustWindowRectEx(&r, style, FALSE, ex);
    *w = r.right - r.left; *h = r.bottom - r.top;
}

static void desiredClient(HMONITOR mon, DWORD style, DWORD ex, int* cw, int* ch) {
    if (g_cfg.windowWidth > 0 && g_cfg.windowHeight > 0) { *cw = g_cfg.windowWidth; *ch = g_cfg.windowHeight; return; }
    // 自動: 内部描画解像度を指定していればそのサイズ、なければモニター解像度を上限に、
    // 作業領域に枠ごと収まる最大の 16:9 標準サイズ
    MONITORINFO mi = { sizeof mi };
    GetMonitorInfoA(mon, &mi);
    int aw = mi.rcWork.right - mi.rcWork.left, ah = mi.rcWork.bottom - mi.rcWork.top;
    int limW = g_cfg.renderWidth > 0 ? g_cfg.renderWidth : mi.rcMonitor.right - mi.rcMonitor.left;
    int limH = g_cfg.renderHeight > 0 ? g_cfg.renderHeight : mi.rcMonitor.bottom - mi.rcMonitor.top;
    static const int sizes[][2] = { { 3840, 2160 }, { 3200, 1800 }, { 2560, 1440 }, { 1920, 1080 }, { 1600, 900 }, { 1280, 720 } };
    for (auto& s : sizes) {
        if (s[0] > limW || s[1] > limH) continue;
        int w, h; frameSize(style, ex, s[0], s[1], &w, &h);
        if (w <= aw && h <= ah) { *cw = s[0]; *ch = s[1]; return; }
    }
    *cw = 1280; *ch = 720;
}

static BOOL WINAPI h_SetWindowPos(HWND h, HWND after, int x, int y, int cx, int cy, UINT f) {
    DWORD st = GetWindowLongA(h, GWL_STYLE), ex = GetWindowLongA(h, GWL_EXSTYLE);
    L("SetWindowPos %p %d,%d %dx%d flags=0x%x style=0x%lx", h, x, y, cx, cy, f, st);
    if (h == g_gameWnd && !(f & SWP_NOSIZE)) {
        HMONITOR mon = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof mi };
        GetMonitorInfoA(mon, &mi);
        int mw = mi.rcMonitor.right - mi.rcMonitor.left, mh = mi.rcMonitor.bottom - mi.rcMonitor.top;
        int nw, nh; frameSize(st, ex, 1280, 720, &nw, &nh);
        if (g_cfg.fixFullscreen && !(st & WS_CAPTION) && cx == mw && cy == mh) {
            // 全画面化: 枠のぶんずれた位置で配置されるのでモニターにぴったり合わせる
            x = mi.rcMonitor.left; y = mi.rcMonitor.top; f &= ~SWP_NOMOVE;
        } else if ((st & WS_CAPTION) && cx == nw && cy == nh) {
            // ウィンドウモードの既定サイズ → 設定サイズにして作業領域の中央へ
            int cw, ch; desiredClient(mon, st, ex, &cw, &ch);
            frameSize(st, ex, cw, ch, &cx, &cy);
            x = mi.rcWork.left + max(0L, ((mi.rcWork.right - mi.rcWork.left) - cx) / 2);
            y = mi.rcWork.top + max(0L, ((mi.rcWork.bottom - mi.rcWork.top) - cy) / 2);
            f &= ~SWP_NOMOVE;
        }
        L("  -> %d,%d %dx%d", x, y, cx, cy);
    }
    return o_SetWindowPos(h, after, x, y, cx, cy, f);
}
static decltype(&CreateWindowExA) o_CreateWindowExA;
static HWND WINAPI h_CreateWindowExA(DWORD ex, LPCSTR cls, LPCSTR name, DWORD st, int x, int y, int w, int h, HWND p, HMENU m, HINSTANCE i, LPVOID lp) {
    // ゲームはウィンドウのクライアントサイズからバックバッファサイズを決めるので、作成時は元のサイズのままにし、
    // デバイス作成後に applyWindowSize で広げる
    HWND r = o_CreateWindowExA(ex, cls, name, st, x, y, w, h, p, m, i, lp);
    if (!p && (st & WS_CAPTION) && !g_gameWnd) g_gameWnd = r;
    L("CreateWindowEx ex=0x%lx style=0x%lx %d,%d %dx%d -> %p", ex, st, x, y, w, h, r);
    return r;
}

// ウィンドウモードのウィンドウを設定サイズにして作業領域の中央へ置く
static void applyWindowSize() {
    HWND h = g_gameWnd ? g_gameWnd : g_hwnd;
    if (!h) return;
    DWORD st = GetWindowLongA(h, GWL_STYLE), ex = GetWindowLongA(h, GWL_EXSTYLE);
    if (!(st & WS_CAPTION)) return;
    HMONITOR mon = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof mi };
    GetMonitorInfoA(mon, &mi);
    int cw, ch, w, hh;
    desiredClient(mon, st, ex, &cw, &ch);
    frameSize(st, ex, cw, ch, &w, &hh);
    int x = mi.rcWork.left + max(0L, ((mi.rcWork.right - mi.rcWork.left) - w) / 2);
    int y = mi.rcWork.top + max(0L, ((mi.rcWork.bottom - mi.rcWork.top) - hh) / 2);
    L("window client %dx%d", cw, ch);
    o_SetWindowPos(h, nullptr, x, y, w, hh, SWP_NOZORDER | SWP_NOACTIVATE);
}

// ゲームはマウスのウィンドウ内座標をそのまま 1280x720 の座標として使い、マウスを閉じ込める範囲も
// ウィンドウ左上から (16,16)-(1264,704) の固定値で計算する。ウィンドウを広げると左上の 1280x720 しか反応しないので、
// ゲームのウィンドウについてだけ、実際のクライアントサイズと 1280x720 の間で座標を変換する
static bool mouseScale(HWND h, float* sx, float* sy) {
    RECT rc;
    if (!h || h != g_gameWnd || !GetClientRect(h, &rc) || rc.right <= 0 || rc.bottom <= 0) return false;
    *sx = (float)rc.right / g_nativeW;
    *sy = (float)rc.bottom / g_nativeH;
    return *sx != 1.0f || *sy != 1.0f;
}

static decltype(&ScreenToClient) o_ScreenToClient;
static BOOL WINAPI h_ScreenToClient(HWND h, LPPOINT pt) {
    BOOL r = o_ScreenToClient(h, pt);
    float sx, sy;
    if (r && mouseScale(h, &sx, &sy)) { pt->x = lroundf(pt->x / sx); pt->y = lroundf(pt->y / sy); }
    return r;
}
static decltype(&ClientToScreen) o_ClientToScreen;
static BOOL WINAPI h_ClientToScreen(HWND h, LPPOINT pt) {
    float sx, sy;
    if (mouseScale(h, &sx, &sy)) { pt->x = lroundf(pt->x * sx); pt->y = lroundf(pt->y * sy); }
    return o_ClientToScreen(h, pt);
}
static decltype(&ClipCursor) o_ClipCursor;
static BOOL WINAPI h_ClipCursor(const RECT* r) {
    float sx, sy;
    POINT origin = { 0, 0 };
    if (r && mouseScale(g_gameWnd, &sx, &sy) && o_ClientToScreen(g_gameWnd, &origin)) {
        // 範囲はクライアント左上を基準に 1280x720 の座標で計算されているので、実際の大きさに広げる
        RECT rc = { origin.x + lroundf((r->left - origin.x) * sx), origin.y + lroundf((r->top - origin.y) * sy),
                    origin.x + lroundf((r->right - origin.x) * sx), origin.y + lroundf((r->bottom - origin.y) * sy) };
        return o_ClipCursor(&rc);
    }
    return o_ClipCursor(r);
}

static void installHooks() {
    o_SetWindowPos = (decltype(o_SetWindowPos))hookIAT("USER32.dll", "SetWindowPos", h_SetWindowPos);
    o_CreateWindowExA = (decltype(o_CreateWindowExA))hookIAT("USER32.dll", "CreateWindowExA", h_CreateWindowExA);
    o_ScreenToClient = (decltype(o_ScreenToClient))hookIAT("USER32.dll", "ScreenToClient", h_ScreenToClient);
    o_ClientToScreen = (decltype(o_ClientToScreen))hookIAT("USER32.dll", "ClientToScreen", h_ClientToScreen);
    o_ClipCursor = (decltype(o_ClipCursor))hookIAT("USER32.dll", "ClipCursor", h_ClipCursor);
}

// ---------------------------------------------------------------- 照準の慣性除去 (exe のメモリパッチ)
// カメラ更新処理 (0x403A5D～) はマウス/パッドの入力をいったん「未消化の回転量」に溜め、
// 毎フレーム camera.speed (0.5度) + 残りの一定割合だけを向きに反映して残りを持ち越す。
// これが照準の慣性になっている。未消化分が speed 以下のときに全量を反映する分岐へ常に飛ばす。
// アドレスは exe の既定ベース (0x400000) 基準。exe は ASLR 対応なので実際のベースに読み替える
struct CodePatch { DWORD va; BYTE orig[8]; BYTE repl[8]; };
static const CodePatch kAimPatches[] = {
    // ヨー: movss xmm1,[ebx+2D0h] → jmp 00403B45 (未消化分を全量 direction に加算して 0 にする)
    { 0x403ABA, { 0xF3, 0x0F, 0x10, 0x8B, 0xD0, 0x02, 0x00, 0x00 }, { 0xE9, 0x86, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90 } },
    // ピッチ: movss xmm1,[ebx+2D0h] → jmp 00403BF4 (未消化分を全量 angle に加算して 0 にする)
    { 0x403B68, { 0xF3, 0x0F, 0x10, 0x8B, 0xD0, 0x02, 0x00, 0x00 }, { 0xE9, 0x87, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90 } },
};

static void applyAimPatch() {
    if (g_cfg.aimSmoothing) return;
    BYTE* base = (BYTE*)GetModuleHandleA(NULL);
    auto at = [&](const CodePatch& p) { return base + (p.va - 0x400000); };
    // 想定した exe でなければ何もしない
    for (auto& p : kAimPatches)
        if (IsBadReadPtr(at(p), 8) || memcmp(at(p), p.orig, 8)) { L("aim patch: unexpected code at %08lx, skipped", p.va); return; }
    for (auto& p : kAimPatches) {
        DWORD old;
        VirtualProtect(at(p), 8, PAGE_EXECUTE_READWRITE, &old);
        memcpy(at(p), p.repl, 8);
        VirtualProtect(at(p), 8, old, &old);
        FlushInstructionCache(GetCurrentProcess(), at(p), 8);
    }
    L("aim smoothing removed (exe base %p)", base);
}

typedef IDirect3D9* (WINAPI* PFN_Create9)(UINT);
extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT v) {
    if (!g_realD3D9) return nullptr;
    IDirect3D9* r = ((PFN_Create9)GetProcAddress(g_realD3D9, "Direct3DCreate9"))(v);
    return r ? new D3D(r) : nullptr;
}

// 転送エクスポート(D3DPERF_* 等)が Direct3DCreate9 より先に呼ばれても動くよう、ロード時に本物を読む
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        loadConfig();
        char path[MAX_PATH];
        GetSystemDirectoryA(path, MAX_PATH);
        strcat(path, "\\d3d9.dll");
        g_realD3D9 = LoadLibraryA(path);
        if (g_realD3D9) initForwards();
        installHooks();
        applyAimPatch();
        // 調査用: ゲームロジックのタイマー周期 (ms) を変える
        if (int ms = GetPrivateProfileIntA("Debug", "LogicIntervalMs", 0, (std::string(g_dir) + "d3d9mod.ini").c_str())) {
            BYTE* p = (BYTE*)GetModuleHandleA(NULL) + (0x4A02BB - 0x400000);
            if (p[0] == 0xC7 && p[1] == 0x05 && *(DWORD*)(p + 6) == 16) {
                DWORD old;
                VirtualProtect(p + 6, 4, PAGE_EXECUTE_READWRITE, &old);
                *(DWORD*)(p + 6) = ms;
                VirtualProtect(p + 6, 4, old, &old);
                L("logic interval %d ms", ms);
            }
        }
        // DPI 仮想化で画面全体が引き伸ばされてぼやけるのを防ぐ
        SetProcessDPIAware();
    }
    return TRUE;
}
