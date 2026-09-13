////////////////////////////////////////////////////////////////////////
// Port of Jan Vlietnick's Julia 4D demo by Wolfgang Engel.
// DirectX 12 compute shader, custom x86 entry point, no C runtime.
////////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "qjulia4D.sh"

#define WINWIDTH 1280
#define WINHEIGHT 720
#define WINPOSX 200
#define WINPOSY 200
#define FRAMECOUNT 2

/* Matches cbCS. Root constants carry the same 128 bytes as the DX11 CB. */
typedef struct MainConstantBuffer {
    float diffuse[4], mu[4], epsilon;
    int width, height, selfShadow;
    float orientation[16], zoom, padding[3];
} MainConstantBuffer;

static MainConstantBuffer mConstants = {
    { 0 }, { 0 }, 0.003f, WINWIDTH, WINHEIGHT, TRUE,
    { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }, 1, { 0 }
};
static float mMuA[4] = { -0.278f, -0.479f, 0, 0 };
static float mMuB[4] = { 0.278f, 0.479f, 0, 0 };
static float mColorA[4] = { 0.25f, 0.45f, 1, 1 };
static float mColorB[4] = { 0.25f, 0.45f, 1, 1 };
static float mPhase;
static UINT mRandom;

static ID3D12Device *mDevice;
static IDXGISwapChain3 *mSwapChain3;
static ID3D12Resource *mRenderTarget[FRAMECOUNT], *mOutput;
static ID3D12CommandQueue *mCommandQueue;
static ID3D12CommandAllocator *mCommandAllocator;
static ID3D12GraphicsCommandList *mCommandList;
static ID3D12DescriptorHeap *mDescriptorHeap;
static D3D12_GPU_DESCRIPTOR_HANDLE mOutputView;
static ID3D12RootSignature *mRootSignature;
static ID3D12PipelineState *mPSO;
static ID3D12Fence *mFence;
static UINT mCurrentFence; /* A 30-second run cannot reach 2^32 frames. */

static void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result)) ExitProcess((UINT)result);
}

/* Same LCG and 24-bit samples in [-1, 1) as the DX11 intro. */
static float GetUniform(void)
{
    mRandom = 1664525u * mRandom + 1013904223u;
    return (float)(mRandom >> 8) * (1.0f / 8388608.0f) - 1.0f;
}

static void Interpolate(float *result, const float *a, const float *b)
{
    UINT n;
    for (n = 0; n < 4; ++n) result[n] = (1.0f - mPhase) * a[n] + mPhase * b[n];
}

static void UpdateAnimation(UINT delta)
{
    UINT n;
    /* Mu and color share the same approximately 20-second interval. */
    mPhase += 0.01f * ((int)delta * (1.0f / 200.0f));
    if (mPhase >= 1.0f) {
        mPhase = 0;
        for (n = 0; n < 4; ++n) {
            mMuA[n] = mMuB[n];
            mMuB[n] = GetUniform();
            mColorA[n] = mColorB[n];
        }
        do {
            for (n = 0; n < 3; ++n) mColorB[n] = GetUniform();
        } while (mColorB[0] < 0 && mColorB[1] < 0 && mColorB[2] < 0);
    }
    Interpolate(mConstants.mu, mMuA, mMuB);
    Interpolate(mConstants.diffuse, mColorA, mColorB);
}

static void InitializeJulia(void)
{
    static D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {
        NULL, { g_CS_QJulia4D, sizeof(g_CS_QJulia4D) }, 0, { 0 }, D3D12_PIPELINE_STATE_FLAG_NONE
    };
    static const D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_DEFAULT, 0, 0, 1, 1 };
    static const D3D12_RESOURCE_DESC texture = {
        D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, WINWIDTH, WINHEIGHT, 1, 1,
        DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };
    static const D3D12_DESCRIPTOR_HEAP_DESC descriptors = {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0
    };
    D3D12_CPU_DESCRIPTOR_HANDLE view;
    /* FXC embeds root signature 1.0; no runtime serializer or compiler DLL. */
    ThrowIfFailed(mDevice->lpVtbl->CreateRootSignature(mDevice, 0, g_CS_QJulia4D, sizeof(g_CS_QJulia4D), &IID_ID3D12RootSignature, (void **)&mRootSignature));
    pipeline.pRootSignature = mRootSignature;
    ThrowIfFailed(mDevice->lpVtbl->CreateComputePipelineState(mDevice, &pipeline, &IID_ID3D12PipelineState, (void **)&mPSO));
    ThrowIfFailed(mDevice->lpVtbl->CreateCommittedResource(mDevice, &heap, 0, &texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, &IID_ID3D12Resource, (void **)&mOutput));
    ThrowIfFailed(mDevice->lpVtbl->CreateDescriptorHeap(mDevice, &descriptors, &IID_ID3D12DescriptorHeap, (void **)&mDescriptorHeap));
    /* x86 C++ COM returns these structs through a hidden output pointer. */
    ((void (__stdcall *)(ID3D12DescriptorHeap *, D3D12_CPU_DESCRIPTOR_HANDLE *))
        mDescriptorHeap->lpVtbl->GetCPUDescriptorHandleForHeapStart)(mDescriptorHeap, &view);
    ((void (__stdcall *)(ID3D12DescriptorHeap *, D3D12_GPU_DESCRIPTOR_HANDLE *))
        mDescriptorHeap->lpVtbl->GetGPUDescriptorHandleForHeapStart)(mDescriptorHeap, &mOutputView);
    mDevice->lpVtbl->CreateUnorderedAccessView(mDevice, mOutput, NULL, NULL, view);
}

static void Transition(ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier = { 0 };
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    mCommandList->lpVtbl->ResourceBarrier(mCommandList, 1, &barrier);
}

static void RecordJulia(ID3D12Resource *target)
{
    ThrowIfFailed(mCommandAllocator->lpVtbl->Reset(mCommandAllocator));
    ThrowIfFailed(mCommandList->lpVtbl->Reset(mCommandList, mCommandAllocator, mPSO));
    mCommandList->lpVtbl->SetComputeRootSignature(mCommandList, mRootSignature);
    mCommandList->lpVtbl->SetComputeRoot32BitConstants(mCommandList, 0, sizeof(mConstants) / 4, &mConstants, 0);
    mCommandList->lpVtbl->SetDescriptorHeaps(mCommandList, 1, &mDescriptorHeap);
    mCommandList->lpVtbl->SetComputeRootDescriptorTable(mCommandList, 1, mOutputView);
    mCommandList->lpVtbl->Dispatch(mCommandList, (WINWIDTH + 3) / 4, (WINHEIGHT + 63) / 64, 1);
    /* The UAV-to-copy transition also orders the compute writes. */
    Transition(mOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    Transition(target, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
    mCommandList->lpVtbl->CopyResource(mCommandList, target, mOutput);
    Transition(target, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
    Transition(mOutput, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

static void WaitForPreviousFrame(void)
{
    ++mCurrentFence;
    ThrowIfFailed(mCommandQueue->lpVtbl->Signal(mCommandQueue, mFence, mCurrentFence));
    /* A null event waits synchronously before allocator/output reuse. */
    ThrowIfFailed(mFence->lpVtbl->SetEventOnCompletion(mFence, mCurrentFence, NULL));
}

__declspec(naked) void __cdecl winmain(void)
{
    __asm {
        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE
    }
    {
        static const D3D12_COMMAND_QUEUE_DESC queueDesc = { 0 };
        static DXGI_SWAP_CHAIN_DESC desc = {
            { WINWIDTH, WINHEIGHT, { 0, 0 }, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0 },
            { 1, 0 }, DXGI_USAGE_RENDER_TARGET_OUTPUT, FRAMECOUNT, NULL, TRUE, DXGI_SWAP_EFFECT_FLIP_DISCARD, 0
        };
        IDXGIFactory4 *factory;
        IDXGISwapChain *swapChain;
        DWORD start, elapsed, previous = 0;
        UINT n, frameIndex;
        MSG message;
        HRESULT result;
        HWND window = CreateWindowW(L"edit", NULL, WS_POPUP | WS_VISIBLE,
            WINPOSX, WINPOSY, WINWIDTH, WINHEIGHT, NULL, NULL, NULL, NULL);
        if (!window) ExitProcess(1);
        ShowCursor(FALSE);
#ifdef _DEBUG
        {
            ID3D12Debug *debug;
            if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, (void **)&debug))) {
                debug->lpVtbl->EnableDebugLayer(debug);
                debug->lpVtbl->Release(debug);
            }
        }
#endif
        ThrowIfFailed(CreateDXGIFactory1(&IID_IDXGIFactory4, (void **)&factory));
        result = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&mDevice);
#ifdef _DEBUG
        if (FAILED(result)) {
            IDXGIAdapter *warp;
            ThrowIfFailed(factory->lpVtbl->EnumWarpAdapter(factory, &IID_IDXGIAdapter, (void **)&warp));
            result = D3D12CreateDevice((IUnknown *)warp, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&mDevice);
            warp->lpVtbl->Release(warp);
        }
#endif
        ThrowIfFailed(result);
        InitializeJulia();
        ThrowIfFailed(mDevice->lpVtbl->CreateCommandQueue(mDevice, &queueDesc, &IID_ID3D12CommandQueue, (void **)&mCommandQueue));
        desc.OutputWindow = window;
        ThrowIfFailed(factory->lpVtbl->CreateSwapChain(factory, (IUnknown *)mCommandQueue, &desc, &swapChain));
        ThrowIfFailed(swapChain->lpVtbl->QueryInterface(swapChain, &IID_IDXGISwapChain3, (void **)&mSwapChain3));
#ifdef _DEBUG
        swapChain->lpVtbl->Release(swapChain);
        factory->lpVtbl->Release(factory);
#endif
        for (n = 0; n < FRAMECOUNT; ++n)
            ThrowIfFailed(mSwapChain3->lpVtbl->GetBuffer(mSwapChain3, n, &IID_ID3D12Resource, (void **)&mRenderTarget[n]));
        ThrowIfFailed(mDevice->lpVtbl->CreateCommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, (void **)&mCommandAllocator));
        ThrowIfFailed(mDevice->lpVtbl->CreateCommandList(mDevice, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, mPSO, &IID_ID3D12GraphicsCommandList, (void **)&mCommandList));
        ThrowIfFailed(mCommandList->lpVtbl->Close(mCommandList));
        ThrowIfFailed(mDevice->lpVtbl->CreateFence(mDevice, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&mFence));
        start = GetTickCount();
        mRandom = start;
        for (;;) {
            while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT || message.message == WM_CLOSE) goto finished;
                DispatchMessageW(&message);
            }
            elapsed = GetTickCount() - start;
            if (elapsed > 30000 || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) break;
            UpdateAnimation(elapsed - previous);
            previous = elapsed;
            /* Occlusion need not advance DXGI's current buffer index. */
            frameIndex = mSwapChain3->lpVtbl->GetCurrentBackBufferIndex(mSwapChain3);
            RecordJulia(mRenderTarget[frameIndex]);
            ThrowIfFailed(mCommandList->lpVtbl->Close(mCommandList));
            mCommandQueue->lpVtbl->ExecuteCommandLists(mCommandQueue, 1, (ID3D12CommandList *const *)&mCommandList);
            ThrowIfFailed(mSwapChain3->lpVtbl->Present(mSwapChain3, 0, 0));
            WaitForPreviousFrame();
        }
finished:
#ifdef _DEBUG
        /* The per-frame fence completed all work before resource release. */
        for (n = 0; n < FRAMECOUNT; ++n) mRenderTarget[n]->lpVtbl->Release(mRenderTarget[n]);
        mCommandList->lpVtbl->Release(mCommandList);
        mCommandAllocator->lpVtbl->Release(mCommandAllocator);
        mOutput->lpVtbl->Release(mOutput);
        mDescriptorHeap->lpVtbl->Release(mDescriptorHeap);
        mSwapChain3->lpVtbl->Release(mSwapChain3);
        mFence->lpVtbl->Release(mFence);
        mCommandQueue->lpVtbl->Release(mCommandQueue);
        mPSO->lpVtbl->Release(mPSO);
        mRootSignature->lpVtbl->Release(mRootSignature);
        mDevice->lpVtbl->Release(mDevice);
        ShowCursor(TRUE);
        DestroyWindow(window);
#else
        ; /* Release relies on process teardown, as in the other tiny demos. */
#endif
    }
    ExitProcess(0);
}
