// 自動生成: 未フックのエクスポートを本物の d3d9.dll へ転送する
#pragma once
extern HMODULE g_realD3D9;
static FARPROC g_fwd[16];
extern "C" __declspec(naked) void fwd_D3DPERF_BeginEvent() { __asm { jmp dword ptr [g_fwd + 0] } }
extern "C" __declspec(naked) void fwd_D3DPERF_EndEvent() { __asm { jmp dword ptr [g_fwd + 4] } }
extern "C" __declspec(naked) void fwd_D3DPERF_GetStatus() { __asm { jmp dword ptr [g_fwd + 8] } }
extern "C" __declspec(naked) void fwd_D3DPERF_QueryRepeatFrame() { __asm { jmp dword ptr [g_fwd + 12] } }
extern "C" __declspec(naked) void fwd_D3DPERF_SetMarker() { __asm { jmp dword ptr [g_fwd + 16] } }
extern "C" __declspec(naked) void fwd_D3DPERF_SetOptions() { __asm { jmp dword ptr [g_fwd + 20] } }
extern "C" __declspec(naked) void fwd_D3DPERF_SetRegion() { __asm { jmp dword ptr [g_fwd + 24] } }
extern "C" __declspec(naked) void fwd_DebugSetLevel() { __asm { jmp dword ptr [g_fwd + 28] } }
extern "C" __declspec(naked) void fwd_DebugSetMute() { __asm { jmp dword ptr [g_fwd + 32] } }
extern "C" __declspec(naked) void fwd_Direct3D9EnableMaximizedWindowedModeShim() { __asm { jmp dword ptr [g_fwd + 36] } }
extern "C" __declspec(naked) void fwd_Direct3DCreate9Ex() { __asm { jmp dword ptr [g_fwd + 40] } }
extern "C" __declspec(naked) void fwd_Direct3DCreate9On12() { __asm { jmp dword ptr [g_fwd + 44] } }
extern "C" __declspec(naked) void fwd_Direct3DCreate9On12Ex() { __asm { jmp dword ptr [g_fwd + 48] } }
extern "C" __declspec(naked) void fwd_Direct3DShaderValidatorCreate9() { __asm { jmp dword ptr [g_fwd + 52] } }
extern "C" __declspec(naked) void fwd_PSGPError() { __asm { jmp dword ptr [g_fwd + 56] } }
extern "C" __declspec(naked) void fwd_PSGPSampleTexture() { __asm { jmp dword ptr [g_fwd + 60] } }
static void initForwards() {
    g_fwd[0] = GetProcAddress(g_realD3D9, "D3DPERF_BeginEvent");
    g_fwd[1] = GetProcAddress(g_realD3D9, "D3DPERF_EndEvent");
    g_fwd[2] = GetProcAddress(g_realD3D9, "D3DPERF_GetStatus");
    g_fwd[3] = GetProcAddress(g_realD3D9, "D3DPERF_QueryRepeatFrame");
    g_fwd[4] = GetProcAddress(g_realD3D9, "D3DPERF_SetMarker");
    g_fwd[5] = GetProcAddress(g_realD3D9, "D3DPERF_SetOptions");
    g_fwd[6] = GetProcAddress(g_realD3D9, "D3DPERF_SetRegion");
    g_fwd[7] = GetProcAddress(g_realD3D9, "DebugSetLevel");
    g_fwd[8] = GetProcAddress(g_realD3D9, "DebugSetMute");
    g_fwd[9] = GetProcAddress(g_realD3D9, "Direct3D9EnableMaximizedWindowedModeShim");
    g_fwd[10] = GetProcAddress(g_realD3D9, "Direct3DCreate9Ex");
    g_fwd[11] = GetProcAddress(g_realD3D9, "Direct3DCreate9On12");
    g_fwd[12] = GetProcAddress(g_realD3D9, "Direct3DCreate9On12Ex");
    g_fwd[13] = GetProcAddress(g_realD3D9, "Direct3DShaderValidatorCreate9");
    g_fwd[14] = GetProcAddress(g_realD3D9, "PSGPError");
    g_fwd[15] = GetProcAddress(g_realD3D9, "PSGPSampleTexture");
}
