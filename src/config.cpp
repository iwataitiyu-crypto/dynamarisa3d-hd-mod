// ダイナマリサ3D 高解像度化 MOD の画質設定ツール
// d3d9mod.ini を編集し、必要ならそのままゲームを起動する
#include <windows.h>
#include <stdio.h>

static wchar_t g_ini[MAX_PATH], g_dir[MAX_PATH];
static HFONT g_font;
static int g_dpi = 96;
static int S(int v) { return MulDiv(v, g_dpi, 96); }

struct Choice { const wchar_t* label; const wchar_t* a; const wchar_t* b; };  // 表示名, 値1, 値2

static const Choice kRender[] = {
    { L"自動 (モニターの解像度)", L"0", L"0" },
    { L"1280 x 720 (オリジナル)", L"1280", L"720" },
    { L"1600 x 900", L"1600", L"900" },
    { L"1920 x 1080 (フルHD)", L"1920", L"1080" },
    { L"2560 x 1440 (WQHD)", L"2560", L"1440" },
    { L"3200 x 1800", L"3200", L"1800" },
    { L"3840 x 2160 (4K)", L"3840", L"2160" },
};
static const Choice kSSAA[] = {
    { L"なし", L"1.0", 0 }, { L"1.5 倍", L"1.5", 0 }, { L"2 倍", L"2.0", 0 },
};
static const Choice kWindow[] = {
    { L"自動 (画面に収まる最大)", L"0", L"0" },
    { L"1280 x 720 (オリジナル)", L"1280", L"720" },
    { L"1600 x 900", L"1600", L"900" },
    { L"1920 x 1080", L"1920", L"1080" },
    { L"2560 x 1440", L"2560", L"1440" },
    { L"3840 x 2160", L"3840", L"2160" },
};
static const Choice kShadow[] = {
    { L"標準 (1024)", L"1", 0 }, { L"高 (2048)", L"2", 0 }, { L"最高 (4096)", L"4", 0 },
};
static const Choice kAniso[] = {
    { L"オフ", L"0", 0 }, { L"4x", L"4", 0 }, { L"8x", L"8", 0 }, { L"16x", L"16", 0 },
};
static const Choice kDof[] = {
    { L"オフ (遠景もくっきり)", L"0.0", 0 }, { L"弱め", L"0.5", 0 }, { L"オリジナル", L"1.0", 0 },
};
static const Choice kAim[] = {
    { L"なし (入力どおりに即反映)", L"0", 0 }, { L"オリジナル (後半にブレーキ)", L"1", 0 },
};
static const Choice kPrecision[] = {
    { L"標準 (1カウント約0.23度)", L"1", 0 }, { L"2倍細かく", L"2", 0 }, { L"3倍細かく", L"3", 0 }, { L"4倍細かく", L"4", 0 },
};
static const Choice kLimit[] = {
    { L"モニターに合わせる (垂直同期)", L"0", 0 }, { L"60 fps", L"60", 0 }, { L"120 fps", L"120", 0 },
    { L"144 fps", L"144", 0 }, { L"165 fps", L"165", 0 }, { L"240 fps", L"240", 0 }, { L"無制限", L"-1", 0 },
};
static const Choice kInterp[] = {
    { L"オフ (ゲーム本来の 60fps)", L"0", 0 }, { L"オン (60fps 超・実験的)", L"1", 0 },
};

enum { ID_RENDER = 100, ID_SSAA, ID_WINDOW, ID_SHADOW, ID_ANISO, ID_DOF, ID_AIM, ID_PRECISION, ID_LIMIT, ID_INTERP, ID_EFFECT, ID_FIXFS, ID_RUN, ID_SAVE, ID_CLOSE };

struct Combo { int id; const wchar_t* section; const wchar_t* keyA; const wchar_t* keyB; const Choice* items; int count; const wchar_t* label; };
static const Combo kCombos[] = {
    { ID_RENDER, L"Graphics", L"RenderWidth", L"RenderHeight", kRender, ARRAYSIZE(kRender), L"内部描画解像度" },
    { ID_SSAA, L"Graphics", L"Supersample", 0, kSSAA, ARRAYSIZE(kSSAA), L"スーパーサンプリング" },
    { ID_WINDOW, L"Window", L"Width", L"Height", kWindow, ARRAYSIZE(kWindow), L"ウィンドウサイズ" },
    { ID_SHADOW, L"Graphics", L"ShadowScale", 0, kShadow, ARRAYSIZE(kShadow), L"影の解像度" },
    { ID_ANISO, L"Graphics", L"Anisotropy", 0, kAniso, ARRAYSIZE(kAniso), L"異方性フィルタ" },
    { ID_DOF, L"Graphics", L"DepthOfField", 0, kDof, ARRAYSIZE(kDof), L"遠景ぼかし (被写界深度)" },
    { ID_AIM, L"Control", L"AimSmoothing", 0, kAim, ARRAYSIZE(kAim), L"照準の慣性" },
    { ID_PRECISION, L"Control", L"MouseAimPrecision", 0, kPrecision, ARRAYSIZE(kPrecision), L"マウス照準の細かさ" },
    { ID_LIMIT, L"Graphics", L"FrameLimit", 0, kLimit, ARRAYSIZE(kLimit), L"フレームレート上限" },
    { ID_INTERP, L"Graphics", L"FrameInterpolation", 0, kInterp, ARRAYSIZE(kInterp), L"フレーム補間" },
};
static const wchar_t* kDefaults[] = { L"0", L"1.0", L"0", L"2", L"16", L"0.0", L"0", L"1", L"0", L"0" };

static HWND ctl(HWND p, const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id) {
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, S(x), S(y), S(w), S(h), p, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}

static void load(HWND w) {
    wchar_t a[32], b[32];
    for (int i = 0; i < ARRAYSIZE(kCombos); i++) {
        const Combo& c = kCombos[i];
        GetPrivateProfileStringW(c.section, c.keyA, kDefaults[i], a, 32, g_ini);
        if (c.keyB) GetPrivateProfileStringW(c.section, c.keyB, L"0", b, 32, g_ini);
        int sel = 0;
        for (int k = 0; k < c.count; k++) {
            if (c.keyB ? !wcscmp(a, c.items[k].a) : _wtof(a) == _wtof(c.items[k].a)) {
                if (!c.keyB || !wcscmp(b, c.items[k].b)) { sel = k; break; }
            }
        }
        SendDlgItemMessageW(w, c.id, CB_SETCURSEL, sel, 0);
    }
    CheckDlgButton(w, ID_EFFECT, GetPrivateProfileIntW(L"Graphics", L"EffectScale", 1, g_ini) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(w, ID_FIXFS, GetPrivateProfileIntW(L"Window", L"FixFullscreen", 1, g_ini) ? BST_CHECKED : BST_UNCHECKED);
}

static void save(HWND w) {
    for (auto& c : kCombos) {
        int sel = (int)SendDlgItemMessageW(w, c.id, CB_GETCURSEL, 0, 0);
        if (sel < 0) sel = 0;
        WritePrivateProfileStringW(c.section, c.keyA, c.items[sel].a, g_ini);
        if (c.keyB) WritePrivateProfileStringW(c.section, c.keyB, c.items[sel].b, g_ini);
    }
    WritePrivateProfileStringW(L"Graphics", L"EffectScale", IsDlgButtonChecked(w, ID_EFFECT) ? L"1" : L"0", g_ini);
    WritePrivateProfileStringW(L"Window", L"FixFullscreen", IsDlgButtonChecked(w, ID_FIXFS) ? L"1" : L"0", g_ini);
}

static LRESULT CALLBACK wndProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_CREATE: {
        int y = 14;
        for (auto& c : kCombos) {
            ctl(w, L"STATIC", c.label, SS_LEFT, 16, y + 4, 150, 20, -1);
            HWND cb = ctl(w, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, 170, y, 230, 300, c.id);
            for (int k = 0; k < c.count; k++) SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)c.items[k].label);
            y += 32;
        }
        ctl(w, L"BUTTON", L"グロー・ぼかし用バッファも高解像度化する", BS_AUTOCHECKBOX | WS_TABSTOP, 16, y, 384, 22, ID_EFFECT); y += 26;
        ctl(w, L"BUTTON", L"全画面化(Alt+Enter)時の位置ずれを補正する", BS_AUTOCHECKBOX | WS_TABSTOP, 16, y, 384, 22, ID_FIXFS); y += 32;
        ctl(w, L"STATIC",
            L"・内部描画解像度は 16:9 で、指定サイズに収まる最大に調整されます。\n"
            L"・内部解像度がウィンドウより大きいと縮小表示されます(AA効果)。\n"
            L"・ゲーム中に Alt+Enter で全画面/ウィンドウを切り替えられます。",
            SS_LEFT, 16, y, 390, 64, -1);
        y += 72;
        ctl(w, L"BUTTON", L"保存して起動", BS_DEFPUSHBUTTON | WS_TABSTOP, 16, y, 130, 30, ID_RUN);
        ctl(w, L"BUTTON", L"保存", BS_PUSHBUTTON | WS_TABSTOP, 156, y, 110, 30, ID_SAVE);
        ctl(w, L"BUTTON", L"閉じる", BS_PUSHBUTTON | WS_TABSTOP, 276, y, 110, 30, ID_CLOSE);
        load(w);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_RUN: {
            save(w);
            wchar_t exe[MAX_PATH];
            swprintf(exe, MAX_PATH, L"%sdynamarisa.exe", g_dir);
            STARTUPINFOW si = { sizeof si };
            PROCESS_INFORMATION pi;
            if (CreateProcessW(exe, NULL, NULL, NULL, FALSE, 0, NULL, g_dir, &si, &pi)) {
                CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
                DestroyWindow(w);
            } else {
                MessageBoxW(w, L"dynamarisa.exe を起動できませんでした。\nこのツールをゲームのフォルダに置いてください。", L"エラー", MB_ICONERROR);
            }
            return 0;
        }
        case ID_SAVE: save(w); MessageBoxW(w, L"設定を保存しました。", L"ダイナマリサ3D HD", MB_ICONINFORMATION); return 0;
        case ID_CLOSE: DestroyWindow(w); return 0;
        }
        break;
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wp, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(w, m, wp, lp);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int show) {
    SetProcessDPIAware();
    HDC dc = GetDC(NULL); g_dpi = GetDeviceCaps(dc, LOGPIXELSY); ReleaseDC(NULL, dc);
    GetModuleFileNameW(NULL, g_dir, MAX_PATH);
    *(wcsrchr(g_dir, L'\\') + 1) = 0;
    swprintf(g_ini, MAX_PATH, L"%sd3d9mod.ini", g_dir);

    NONCLIENTMETRICSW ncm = { sizeof ncm };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0);
    g_font = CreateFontIndirectW(&ncm.lfMessageFont);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = L"DynamarisaHDConfig";
    RegisterClassW(&wc);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT r = { 0, 0, S(420), S(32 * ARRAYSIZE(kCombos) + 14 + 26 + 32 + 72 + 30 + 16) };
    AdjustWindowRect(&r, style, FALSE);
    int ww = r.right - r.left, wh = r.bottom - r.top;
    HWND w = CreateWindowW(wc.lpszClassName, L"ダイナマリサ3D 画質設定 (HD MOD)", style,
                           (GetSystemMetrics(SM_CXSCREEN) - ww) / 2, (GetSystemMetrics(SM_CYSCREEN) - wh) / 2, ww, wh,
                           NULL, NULL, inst, NULL);
    ShowWindow(w, show);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (IsDialogMessageW(w, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
