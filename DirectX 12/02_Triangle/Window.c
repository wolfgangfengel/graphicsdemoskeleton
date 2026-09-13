////////////////////////////////////////////////////////////////////////
// Skeleton Intro Coding
// by Wolfgang Engel; started in 2003 or earlier.
// DirectX 12 triangle, custom x86 entry point, no C runtime.
////////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "vertex.sh"
#include "pixel.sh"

#define WINWIDTH 800
#define WINHEIGHT 600
#define WINPOSX 200
#define WINPOSY 200
#define FRAMECOUNT 2

static IDXGISwapChain3 *mSwapChain3;
static ID3D12Device *mDevice;
static ID3D12Resource *mRenderTarget[FRAMECOUNT];
static D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView[FRAMECOUNT];
static ID3D12CommandAllocator *mCommandAllocator;
static ID3D12CommandQueue *mCommandQueue;
static ID3D12DescriptorHeap *mDescriptorHeap;
static ID3D12GraphicsCommandList *mCommandList;
static ID3D12RootSignature *mRootSignature;
static ID3D12PipelineState *mPSO;
static ID3D12Fence *mFence;
static UINT mCurrentFence; /* The 3.3-second run cannot reach 2^32 frames. */

/* Row-major storage and row vectors: position * World * View * Projection. */
typedef struct Matrix4 { float m[4][4]; } Matrix4;
static Matrix4 mWorld = {{
    { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 }
}};
/* Left-handed camera at (0, 0, -2), looking toward +Z, with +Y up. */
static const Matrix4 mView = {{
    { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 2, 1 }
}};
/* Perspective: 60-degree vertical FOV, 800/600 aspect, near 0.1, far 10.
 * cot(FOV/2) = sqrt(3); Direct3D maps depth into [0, 1]. */
static const Matrix4 mProjection = {{
    { 1.7320508f * WINHEIGHT / WINWIDTH, 0, 0, 0 },
    { 0, 1.7320508f, 0, 0 },
    { 0, 0, 10.0f / (10.0f - 0.1f), 1 },
    { 0, 0, -0.1f * 10.0f / (10.0f - 0.1f), 0 }
}};
static Matrix4 mWorldViewProjection;

/* The destination must be distinct from both inputs. */
static void MultiplyMatrices(Matrix4 *result, const Matrix4 *left, const Matrix4 *right)
{
    UINT row, column, k;
    for (row = 0; row < 4; ++row) {
        for (column = 0; column < 4; ++column) {
            float value = 0;
            for (k = 0; k < 4; ++k) value += left->m[row][k] * right->m[k][column];
            result->m[row][column] = value;
        }
    }
}

static void UpdateTransform(UINT elapsed)
{
    Matrix4 worldView;
    float angle = (int)elapsed * 0.002f; /* Two radians per second around Y. */
    float s, c;
    /* x87 keeps this x86 demo independent of a CRT trigonometry library. */
    __asm {
        fld angle
        fsincos
        fstp c
        fstp s
    }
    mWorld.m[0][0] = mWorld.m[2][2] = c;
    mWorld.m[0][2] = -s;
    mWorld.m[2][0] = s;
    MultiplyMatrices(&worldView, &mWorld, &mView);
    MultiplyMatrices(&mWorldViewProjection, &worldView, &mProjection);
}

static void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) ExitProcess((UINT)hr);
}

static void WaitForPreviousFrame(void)
{
    /* Wait after Present before reusing the allocator and list next frame.
     * A null event waits synchronously for the signaled fence value. */
    ++mCurrentFence;
    ThrowIfFailed(mCommandQueue->lpVtbl->Signal(mCommandQueue, mFence, mCurrentFence));
    ThrowIfFailed(mFence->lpVtbl->SetEventOnCompletion(mFence, mCurrentFence, NULL));
}

static void InitializeTriangle(void)
{
    /* Static descriptors compress better than assignments and structure copies. */
    static D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline = {
        NULL, { g_VShader, sizeof(g_VShader) }, { g_PShader, sizeof(g_PShader) }, { 0 }, { 0 }, { 0 }, { 0 },
        { FALSE, FALSE, {{ FALSE, FALSE, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL }} },
        UINT_MAX,
        /* Show both faces as the triangle rotates away from the camera. */
        { D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, FALSE, 0, 0, 0, TRUE, FALSE, FALSE, 0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF },
        { 0 }, /* Depth and stencil are disabled. */
        { NULL, 0 }, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        1, { DXGI_FORMAT_R8G8B8A8_UNORM }, DXGI_FORMAT_UNKNOWN, { 1, 0 }, 0, { 0 }, D3D12_PIPELINE_STATE_FLAG_NONE
    };
    /* FXC embeds the root signature in the vertex shader. No runtime
     * serializer, vertex buffer, or input layout is needed with SV_VertexID. */
    ThrowIfFailed(mDevice->lpVtbl->CreateRootSignature(mDevice, 0, g_VShader, sizeof(g_VShader), &IID_ID3D12RootSignature, (void **)&mRootSignature));
    pipeline.pRootSignature = mRootSignature;
    ThrowIfFailed(mDevice->lpVtbl->CreateGraphicsPipelineState(mDevice, &pipeline, &IID_ID3D12PipelineState, (void **)&mPSO));
}
static void RecordTriangle(ID3D12Resource *target, D3D12_CPU_DESCRIPTOR_HANDLE view, UINT elapsed)
{
    static const D3D12_VIEWPORT viewport = { 0, 0, WINWIDTH, WINHEIGHT, 0, 1 };
    static const D3D12_RECT scissor = { 0, 0, WINWIDTH, WINHEIGHT };
    static const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    static D3D12_RESOURCE_BARRIER barrier = {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
        { NULL, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, 0, 0 }
    };
    ThrowIfFailed(mCommandAllocator->lpVtbl->Reset(mCommandAllocator));
    ThrowIfFailed(mCommandList->lpVtbl->Reset(mCommandList, mCommandAllocator, mPSO));
    mCommandList->lpVtbl->SetGraphicsRootSignature(mCommandList, mRootSignature);
    UpdateTransform(elapsed);
    /* The shader declares row_major, so upload the concatenated matrix as-is. */
    mCommandList->lpVtbl->SetGraphicsRoot32BitConstants(mCommandList, 0, 16, &mWorldViewProjection, 0);
    mCommandList->lpVtbl->RSSetViewports(mCommandList, 1, &viewport);
    mCommandList->lpVtbl->RSSetScissorRects(mCommandList, 1, &scissor);
    barrier.Transition.pResource = target;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    mCommandList->lpVtbl->ResourceBarrier(mCommandList, 1, &barrier);
    mCommandList->lpVtbl->ClearRenderTargetView(mCommandList, view, clearColor, 0, NULL);
    mCommandList->lpVtbl->OMSetRenderTargets(mCommandList, 1, &view, TRUE, NULL);
    mCommandList->lpVtbl->IASetPrimitiveTopology(mCommandList, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->lpVtbl->DrawInstanced(mCommandList, 3, 1, 0, 0);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    mCommandList->lpVtbl->ResourceBarrier(mCommandList, 1, &barrier);
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
        static const D3D12_DESCRIPTOR_HEAP_DESC heapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAMECOUNT, 0, 0 };
        static DXGI_SWAP_CHAIN_DESC desc = {
            { WINWIDTH, WINHEIGHT, { 0, 0 }, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0 },
            { 1, 0 }, DXGI_USAGE_RENDER_TARGET_OUTPUT, FRAMECOUNT, NULL, TRUE, DXGI_SWAP_EFFECT_FLIP_DISCARD, 0
        };
        IDXGIFactory4 *factory;
        IDXGISwapChain *swapChain;
        D3D12_CPU_DESCRIPTOR_HANDLE view;
        UINT n, increment, frameIndex, elapsed;
        DWORD start;
        MSG message;
        HRESULT result;
        HWND window = CreateWindowW(L"edit", NULL, WS_POPUP | WS_VISIBLE, WINPOSX, WINPOSY, WINWIDTH, WINHEIGHT, NULL, NULL, NULL, NULL);
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
        InitializeTriangle();
        ThrowIfFailed(mDevice->lpVtbl->CreateCommandQueue(mDevice, &queueDesc, &IID_ID3D12CommandQueue, (void **)&mCommandQueue));
        desc.OutputWindow = window;
        ThrowIfFailed(factory->lpVtbl->CreateSwapChain(factory, (IUnknown *)mCommandQueue, &desc, &swapChain));
        ThrowIfFailed(swapChain->lpVtbl->QueryInterface(swapChain, &IID_IDXGISwapChain3, (void **)&mSwapChain3));
#ifdef _DEBUG
        swapChain->lpVtbl->Release(swapChain);
        factory->lpVtbl->Release(factory);
#endif
        ThrowIfFailed(mDevice->lpVtbl->CreateDescriptorHeap(mDevice, &heapDesc, &IID_ID3D12DescriptorHeap, (void **)&mDescriptorHeap));
        increment = mDevice->lpVtbl->GetDescriptorHandleIncrementSize(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        /* The x86 C++ COM implementation returns the handle through a hidden
         * output pointer; the SDK C declaration needs this ABI workaround. */
        ((void (__stdcall *)(ID3D12DescriptorHeap *, D3D12_CPU_DESCRIPTOR_HANDLE *))
            mDescriptorHeap->lpVtbl->GetCPUDescriptorHandleForHeapStart)(mDescriptorHeap, &view);
        for (n = 0; n < FRAMECOUNT; ++n) {
            ThrowIfFailed(mSwapChain3->lpVtbl->GetBuffer(mSwapChain3, n, &IID_ID3D12Resource, (void **)&mRenderTarget[n]));
            mDevice->lpVtbl->CreateRenderTargetView(mDevice, mRenderTarget[n], NULL, view);
            mRenderTargetView[n] = view;
            view.ptr += increment;
        }
        ThrowIfFailed(mDevice->lpVtbl->CreateCommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, (void **)&mCommandAllocator));
        ThrowIfFailed(mDevice->lpVtbl->CreateCommandList(mDevice, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, mPSO, &IID_ID3D12GraphicsCommandList, (void **)&mCommandList));
        ThrowIfFailed(mCommandList->lpVtbl->Close(mCommandList));
        ThrowIfFailed(mDevice->lpVtbl->CreateFence(mDevice, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&mFence));
        start = GetTickCount();
        for (;;) {
            while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT || message.message == WM_CLOSE) ExitProcess(0);
                DispatchMessageW(&message);
            }
            elapsed = GetTickCount() - start;
            if (elapsed > 3300 || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) break;
            /* Query DXGI instead of assuming that Present always advances. */
            frameIndex = mSwapChain3->lpVtbl->GetCurrentBackBufferIndex(mSwapChain3);
            RecordTriangle(mRenderTarget[frameIndex], mRenderTargetView[frameIndex], elapsed);
            ThrowIfFailed(mCommandList->lpVtbl->Close(mCommandList));
            mCommandQueue->lpVtbl->ExecuteCommandLists(mCommandQueue, 1, (ID3D12CommandList *const *)&mCommandList);
            ThrowIfFailed(mSwapChain3->lpVtbl->Present(mSwapChain3, 1, 0));
            WaitForPreviousFrame();
        }
#ifdef _DEBUG
        /* The last per-frame fence also completed the final Present. */
        for (n = 0; n < FRAMECOUNT; ++n) {
            mRenderTarget[n]->lpVtbl->Release(mRenderTarget[n]);
        }
        mCommandList->lpVtbl->Release(mCommandList);
        mCommandAllocator->lpVtbl->Release(mCommandAllocator);
        mDescriptorHeap->lpVtbl->Release(mDescriptorHeap);
        mSwapChain3->lpVtbl->Release(mSwapChain3);
        mFence->lpVtbl->Release(mFence);
        mCommandQueue->lpVtbl->Release(mCommandQueue);
        mPSO->lpVtbl->Release(mPSO);
        mRootSignature->lpVtbl->Release(mRootSignature);
        mDevice->lpVtbl->Release(mDevice);
        ShowCursor(TRUE);
        DestroyWindow(window);
#endif
    }
    ExitProcess(0);
}
