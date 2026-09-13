////////////////////////////////////////////////////////////////////////
// Skeleton Intro Coding
// by Wolfgang Engel; started in 2003 or earlier.
// DirectX 12, custom x86 entry point, no C runtime.
////////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#define WINWIDTH 800
#define WINHEIGHT 600
#define WINPOSX 200
#define WINPOSY 200
#define FRAMECOUNT 2

static IDXGISwapChain3 *mSwapChain3;
static ID3D12Device *mDevice;
static ID3D12Resource *mRenderTarget[FRAMECOUNT];
static ID3D12CommandAllocator *mCommandAllocator;
static ID3D12CommandQueue *mCommandQueue;
static ID3D12DescriptorHeap *mDescriptorHeap;
static ID3D12GraphicsCommandList *mCommandList;
static ID3D12Fence *mFence;
static D3D12_CPU_DESCRIPTOR_HANDLE mRenderView[FRAMECOUNT];
static UINT mCurrentFence; /* The 3.3-second run cannot reach 2^32 frames. */

static void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) ExitProcess((UINT)hr);
}

static void WaitForPreviousFrame(void)
{
    /* One frame in flight keeps allocator reuse safe and the code small.
     * A null event waits synchronously for the signaled fence value. */
    ++mCurrentFence;
    ThrowIfFailed(mCommandQueue->lpVtbl->Signal(mCommandQueue, mFence, mCurrentFence));
    ThrowIfFailed(mFence->lpVtbl->SetEventOnCompletion(mFence, mCurrentFence, NULL));
}

static void RecordClear(ID3D12Resource *target, D3D12_CPU_DESCRIPTOR_HANDLE view)
{
    static const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    static D3D12_RESOURCE_BARRIER barrier = {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
        { NULL, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, 0, 0 }
    };
    ThrowIfFailed(mCommandAllocator->lpVtbl->Reset(mCommandAllocator));
    ThrowIfFailed(mCommandList->lpVtbl->Reset(mCommandList, mCommandAllocator, NULL));
    barrier.Transition.pResource = target;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    mCommandList->lpVtbl->ResourceBarrier(mCommandList, 1, &barrier);
    mCommandList->lpVtbl->ClearRenderTargetView(mCommandList, view, clearColor, 0, NULL);
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
        UINT n, increment, frameIndex;
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
            mRenderView[n] = view;
            view.ptr += increment;
        }
        /* A clear needs neither shaders nor a pipeline state/root signature. */
        ThrowIfFailed(mDevice->lpVtbl->CreateCommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, (void **)&mCommandAllocator));
        ThrowIfFailed(mDevice->lpVtbl->CreateCommandList(mDevice, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, NULL, &IID_ID3D12GraphicsCommandList, (void **)&mCommandList));
        ThrowIfFailed(mCommandList->lpVtbl->Close(mCommandList));
        ThrowIfFailed(mDevice->lpVtbl->CreateFence(mDevice, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&mFence));
        start = GetTickCount();
        for (;;) {
            while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT || message.message == WM_CLOSE) ExitProcess(0);
                DispatchMessageW(&message);
            }
            if (GetTickCount() - start > 3300 || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) break;
            /* Query DXGI instead of assuming that Present always advances. */
            frameIndex = mSwapChain3->lpVtbl->GetCurrentBackBufferIndex(mSwapChain3);
            RecordClear(mRenderTarget[frameIndex], mRenderView[frameIndex]);
            ThrowIfFailed(mCommandList->lpVtbl->Close(mCommandList));
            mCommandQueue->lpVtbl->ExecuteCommandLists(mCommandQueue, 1, (ID3D12CommandList *const *)&mCommandList);
            ThrowIfFailed(mSwapChain3->lpVtbl->Present(mSwapChain3, 1, 0));
            WaitForPreviousFrame();
        }
#ifdef _DEBUG
        /* The final frame has already completed. Release reclaims everything
         * through ExitProcess; Debug keeps explicit teardown for inspection. */
        for (n = 0; n < FRAMECOUNT; ++n) mRenderTarget[n]->lpVtbl->Release(mRenderTarget[n]);
        mCommandList->lpVtbl->Release(mCommandList);
        mCommandAllocator->lpVtbl->Release(mCommandAllocator);
        mDescriptorHeap->lpVtbl->Release(mDescriptorHeap);
        mSwapChain3->lpVtbl->Release(mSwapChain3);
        mFence->lpVtbl->Release(mFence);
        mCommandQueue->lpVtbl->Release(mCommandQueue);
        mDevice->lpVtbl->Release(mDevice);
        ShowCursor(TRUE);
        DestroyWindow(window);
#endif
    }
    ExitProcess(0);
}
