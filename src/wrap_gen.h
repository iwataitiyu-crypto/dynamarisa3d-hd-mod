// 自動生成: gen_wrappers.py
#pragma once
#include <d3d9.h>

struct D3D9Base : IDirect3D9 {
    IDirect3D9* real;
    D3D9Base(IDirect3D9* r) : real(r) {}
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppv) override { return real->QueryInterface(riid, ppv); }
    STDMETHOD_(ULONG, AddRef)(THIS) override { return real->AddRef(); }
    STDMETHOD_(ULONG, Release)(THIS) override { ULONG r = real->Release(); if (r == 0) delete this; return r; }
    STDMETHOD(RegisterSoftwareDevice)(THIS_ void* pInitializeFunction) override { return real->RegisterSoftwareDevice(pInitializeFunction); }
    STDMETHOD_(UINT, GetAdapterCount)(THIS) override { return real->GetAdapterCount(); }
    STDMETHOD(GetAdapterIdentifier)(THIS_ UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) override { return real->GetAdapterIdentifier(Adapter, Flags, pIdentifier); }
    STDMETHOD_(UINT, GetAdapterModeCount)(THIS_ UINT Adapter, D3DFORMAT Format) override { return real->GetAdapterModeCount(Adapter, Format); }
    STDMETHOD(EnumAdapterModes)(THIS_ UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) override { return real->EnumAdapterModes(Adapter, Format, Mode, pMode); }
    STDMETHOD(GetAdapterDisplayMode)(THIS_ UINT Adapter, D3DDISPLAYMODE* pMode) override { return real->GetAdapterDisplayMode(Adapter, pMode); }
    STDMETHOD(CheckDeviceType)(THIS_ UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed) override { return real->CheckDeviceType(Adapter, DevType, AdapterFormat, BackBufferFormat, bWindowed); }
    STDMETHOD(CheckDeviceFormat)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) override { return real->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat); }
    STDMETHOD(CheckDeviceMultiSampleType)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) override { return real->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels); }
    STDMETHOD(CheckDepthStencilMatch)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) override { return real->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat); }
    STDMETHOD(CheckDeviceFormatConversion)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) override { return real->CheckDeviceFormatConversion(Adapter, DeviceType, SourceFormat, TargetFormat); }
    STDMETHOD(GetDeviceCaps)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) override { return real->GetDeviceCaps(Adapter, DeviceType, pCaps); }
    STDMETHOD_(HMONITOR, GetAdapterMonitor)(THIS_ UINT Adapter) override { return real->GetAdapterMonitor(Adapter); }
    STDMETHOD(CreateDevice)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) override { return real->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface); }
};

struct DeviceBase : IDirect3DDevice9 {
    IDirect3DDevice9* real;
    DeviceBase(IDirect3DDevice9* r) : real(r) {}
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppv) override { return real->QueryInterface(riid, ppv); }
    STDMETHOD_(ULONG, AddRef)(THIS) override { return real->AddRef(); }
    STDMETHOD_(ULONG, Release)(THIS) override { ULONG r = real->Release(); if (r == 0) delete this; return r; }
    STDMETHOD(TestCooperativeLevel)(THIS) override { return real->TestCooperativeLevel(); }
    STDMETHOD_(UINT, GetAvailableTextureMem)(THIS) override { return real->GetAvailableTextureMem(); }
    STDMETHOD(EvictManagedResources)(THIS) override { return real->EvictManagedResources(); }
    STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9) override { return real->GetDirect3D(ppD3D9); }
    STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps) override { return real->GetDeviceCaps(pCaps); }
    STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain, D3DDISPLAYMODE* pMode) override { return real->GetDisplayMode(iSwapChain, pMode); }
    STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters) override { return real->GetCreationParameters(pParameters); }
    STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) override { return real->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap); }
    STDMETHOD_(void, SetCursorPosition)(THIS_ int X, int Y, DWORD Flags) override { return real->SetCursorPosition(X, Y, Flags); }
    STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow) override { return real->ShowCursor(bShow); }
    STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) override { return real->CreateAdditionalSwapChain(pPresentationParameters, pSwapChain); }
    STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) override { return real->GetSwapChain(iSwapChain, pSwapChain); }
    STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS) override { return real->GetNumberOfSwapChains(); }
    STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) override { return real->Reset(pPresentationParameters); }
    STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) override { return real->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion); }
    STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) override { return real->GetBackBuffer(iSwapChain, iBackBuffer, Type, ppBackBuffer); }
    STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) override { return real->GetRasterStatus(iSwapChain, pRasterStatus); }
    STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs) override { return real->SetDialogBoxMode(bEnableDialogs); }
    STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp) override { return real->SetGammaRamp(iSwapChain, Flags, pRamp); }
    STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain, D3DGAMMARAMP* pRamp) override { return real->GetGammaRamp(iSwapChain, pRamp); }
    STDMETHOD(CreateTexture)(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) override { return real->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle); }
    STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) override { return real->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle); }
    STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) override { return real->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle); }
    STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) override { return real->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle); }
    STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) override { return real->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle); }
    STDMETHOD(CreateRenderTarget)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override { return real->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle); }
    STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override { return real->CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle); }
    STDMETHOD(UpdateSurface)(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) override { return real->UpdateSurface(pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint); }
    STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) override { return real->UpdateTexture(pSourceTexture, pDestinationTexture); }
    STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) override { return real->GetRenderTargetData(pRenderTarget, pDestSurface); }
    STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain, IDirect3DSurface9* pDestSurface) override { return real->GetFrontBufferData(iSwapChain, pDestSurface); }
    STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) override { return real->StretchRect(pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter); }
    STDMETHOD(ColorFill)(THIS_ IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color) override { return real->ColorFill(pSurface, pRect, color); }
    STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override { return real->CreateOffscreenPlainSurface(Width, Height, Format, Pool, ppSurface, pSharedHandle); }
    STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) override { return real->SetRenderTarget(RenderTargetIndex, pRenderTarget); }
    STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) override { return real->GetRenderTarget(RenderTargetIndex, ppRenderTarget); }
    STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* pNewZStencil) override { return real->SetDepthStencilSurface(pNewZStencil); }
    STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface9** ppZStencilSurface) override { return real->GetDepthStencilSurface(ppZStencilSurface); }
    STDMETHOD(BeginScene)(THIS) override { return real->BeginScene(); }
    STDMETHOD(EndScene)(THIS) override { return real->EndScene(); }
    STDMETHOD(Clear)(THIS_ DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) override { return real->Clear(Count, pRects, Flags, Color, Z, Stencil); }
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) override { return real->SetTransform(State, pMatrix); }
    STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) override { return real->GetTransform(State, pMatrix); }
    STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE unnamed0, CONST D3DMATRIX* unnamed1) override { return real->MultiplyTransform(unnamed0, unnamed1); }
    STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport) override { return real->SetViewport(pViewport); }
    STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport) override { return real->GetViewport(pViewport); }
    STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial) override { return real->SetMaterial(pMaterial); }
    STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial) override { return real->GetMaterial(pMaterial); }
    STDMETHOD(SetLight)(THIS_ DWORD Index, CONST D3DLIGHT9* unnamed1) override { return real->SetLight(Index, unnamed1); }
    STDMETHOD(GetLight)(THIS_ DWORD Index, D3DLIGHT9* unnamed1) override { return real->GetLight(Index, unnamed1); }
    STDMETHOD(LightEnable)(THIS_ DWORD Index, BOOL Enable) override { return real->LightEnable(Index, Enable); }
    STDMETHOD(GetLightEnable)(THIS_ DWORD Index, BOOL* pEnable) override { return real->GetLightEnable(Index, pEnable); }
    STDMETHOD(SetClipPlane)(THIS_ DWORD Index, CONST float* pPlane) override { return real->SetClipPlane(Index, pPlane); }
    STDMETHOD(GetClipPlane)(THIS_ DWORD Index, float* pPlane) override { return real->GetClipPlane(Index, pPlane); }
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State, DWORD Value) override { return real->SetRenderState(State, Value); }
    STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State, DWORD* pValue) override { return real->GetRenderState(State, pValue); }
    STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) override { return real->CreateStateBlock(Type, ppSB); }
    STDMETHOD(BeginStateBlock)(THIS) override { return real->BeginStateBlock(); }
    STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB) override { return real->EndStateBlock(ppSB); }
    STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus) override { return real->SetClipStatus(pClipStatus); }
    STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus) override { return real->GetClipStatus(pClipStatus); }
    STDMETHOD(GetTexture)(THIS_ DWORD Stage, IDirect3DBaseTexture9** ppTexture) override { return real->GetTexture(Stage, ppTexture); }
    STDMETHOD(SetTexture)(THIS_ DWORD Stage, IDirect3DBaseTexture9* pTexture) override { return real->SetTexture(Stage, pTexture); }
    STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) override { return real->GetTextureStageState(Stage, Type, pValue); }
    STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override { return real->SetTextureStageState(Stage, Type, Value); }
    STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) override { return real->GetSamplerState(Sampler, Type, pValue); }
    STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) override { return real->SetSamplerState(Sampler, Type, Value); }
    STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses) override { return real->ValidateDevice(pNumPasses); }
    STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber, CONST PALETTEENTRY* pEntries) override { return real->SetPaletteEntries(PaletteNumber, pEntries); }
    STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber, PALETTEENTRY* pEntries) override { return real->GetPaletteEntries(PaletteNumber, pEntries); }
    STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber) override { return real->SetCurrentTexturePalette(PaletteNumber); }
    STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT *PaletteNumber) override { return real->GetCurrentTexturePalette(PaletteNumber); }
    STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect) override { return real->SetScissorRect(pRect); }
    STDMETHOD(GetScissorRect)(THIS_ RECT* pRect) override { return real->GetScissorRect(pRect); }
    STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware) override { return real->SetSoftwareVertexProcessing(bSoftware); }
    STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS) override { return real->GetSoftwareVertexProcessing(); }
    STDMETHOD(SetNPatchMode)(THIS_ float nSegments) override { return real->SetNPatchMode(nSegments); }
    STDMETHOD_(float, GetNPatchMode)(THIS) override { return real->GetNPatchMode(); }
    STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override { return real->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount); }
    STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE unnamed0, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override { return real->DrawIndexedPrimitive(unnamed0, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount); }
    STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override { return real->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride); }
    STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override { return real->DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride); }
    STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) override { return real->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags); }
    STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) override { return real->CreateVertexDeclaration(pVertexElements, ppDecl); }
    STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl) override { return real->SetVertexDeclaration(pDecl); }
    STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl) override { return real->GetVertexDeclaration(ppDecl); }
    STDMETHOD(SetFVF)(THIS_ DWORD FVF) override { return real->SetFVF(FVF); }
    STDMETHOD(GetFVF)(THIS_ DWORD* pFVF) override { return real->GetFVF(pFVF); }
    STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) override { return real->CreateVertexShader(pFunction, ppShader); }
    STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader) override { return real->SetVertexShader(pShader); }
    STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader) override { return real->GetVertexShader(ppShader); }
    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) override { return real->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount); }
    STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount) override { return real->GetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount); }
    STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) override { return real->SetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount); }
    STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount) override { return real->GetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount); }
    STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) override { return real->SetVertexShaderConstantB(StartRegister, pConstantData, BoolCount); }
    STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override { return real->GetVertexShaderConstantB(StartRegister, pConstantData, BoolCount); }
    STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) override { return real->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride); }
    STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride) override { return real->GetStreamSource(StreamNumber, ppStreamData, pOffsetInBytes, pStride); }
    STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber, UINT Setting) override { return real->SetStreamSourceFreq(StreamNumber, Setting); }
    STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber, UINT* pSetting) override { return real->GetStreamSourceFreq(StreamNumber, pSetting); }
    STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* pIndexData) override { return real->SetIndices(pIndexData); }
    STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer9** ppIndexData) override { return real->GetIndices(ppIndexData); }
    STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) override { return real->CreatePixelShader(pFunction, ppShader); }
    STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader) override { return real->SetPixelShader(pShader); }
    STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader) override { return real->GetPixelShader(ppShader); }
    STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) override { return real->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount); }
    STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount) override { return real->GetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount); }
    STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) override { return real->SetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount); }
    STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount) override { return real->GetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount); }
    STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) override { return real->SetPixelShaderConstantB(StartRegister, pConstantData, BoolCount); }
    STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override { return real->GetPixelShaderConstantB(StartRegister, pConstantData, BoolCount); }
    STDMETHOD(DrawRectPatch)(THIS_ UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) override { return real->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo); }
    STDMETHOD(DrawTriPatch)(THIS_ UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) override { return real->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo); }
    STDMETHOD(DeletePatch)(THIS_ UINT Handle) override { return real->DeletePatch(Handle); }
    STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) override { return real->CreateQuery(Type, ppQuery); }
};

struct SwapChainBase : IDirect3DSwapChain9 {
    IDirect3DSwapChain9* real;
    SwapChainBase(IDirect3DSwapChain9* r) : real(r) {}
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppv) override { return real->QueryInterface(riid, ppv); }
    STDMETHOD_(ULONG, AddRef)(THIS) override { return real->AddRef(); }
    STDMETHOD_(ULONG, Release)(THIS) override { ULONG r = real->Release(); if (r == 0) delete this; return r; }
    STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) override { return real->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags); }
    STDMETHOD(GetFrontBufferData)(THIS_ IDirect3DSurface9* pDestSurface) override { return real->GetFrontBufferData(pDestSurface); }
    STDMETHOD(GetBackBuffer)(THIS_ UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) override { return real->GetBackBuffer(iBackBuffer, Type, ppBackBuffer); }
    STDMETHOD(GetRasterStatus)(THIS_ D3DRASTER_STATUS* pRasterStatus) override { return real->GetRasterStatus(pRasterStatus); }
    STDMETHOD(GetDisplayMode)(THIS_ D3DDISPLAYMODE* pMode) override { return real->GetDisplayMode(pMode); }
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) override { return real->GetDevice(ppDevice); }
    STDMETHOD(GetPresentParameters)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) override { return real->GetPresentParameters(pPresentationParameters); }
};

