////////////////////////////////////////////////////////////////////////
//
// Skeleton Intro Coding
//
// by Wolfgang Engel 
// Last time modified: 05/21/2015
//
///////////////////////////////////////////////////////////////////////

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_4.h>
//#include <D3Dcompiler.h>
//#include <DirectXMath.h>
#include "d3dx12.h"

//#include <string>
#include <wrl.h> // use namespace Microsoft::WRL;

using namespace Microsoft::WRL;

// define the size of the window
#define WINWIDTH 800 
#define WINHEIGHT 600
#define WINPOSX 200 
#define WINPOSY 200

int mIndexLastSwapBuf = 0;
int cNumSwapBufs = 2;


// Pipeline objects.
ComPtr<IDXGISwapChain> mSwapChain;
ComPtr<ID3D12Device> mDevice;
ComPtr<ID3D12Resource> mRenderTarget;
ComPtr<ID3D12CommandAllocator> mCommandAllocator;
ComPtr<ID3D12CommandQueue> mCommandQueue;
ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
ComPtr<ID3D12PipelineState> mPSO;
ComPtr<ID3D12GraphicsCommandList> mCommandList;

// Synchronization objects.
HANDLE mHandleEvent;
ComPtr<ID3D12Fence> mFence;
UINT64 mCurrentFence;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

void WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. More advanced samples 
	// illustrate how to use fences for efficient resource usage.

	// Signal and increment the fence value.
	const UINT64 fence = mCurrentFence;
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), fence));
	mCurrentFence++;

	// Wait until the previous frame is finished.
	if (mFence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(fence, mHandleEvent));
		WaitForSingleObject(mHandleEvent, INFINITE);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// timer global variables
	DWORD		StartTime;
	DWORD		CurrentTime;

	// keep track if the game loop is still running
	BOOL		BRunning;

	// the most simple window
	HWND hWnd = CreateWindow(L"edit", 0, WS_POPUP | WS_VISIBLE, WINPOSX, WINPOSY, WINWIDTH, WINHEIGHT, 0, 0, 0, 0);

	// don't show the cursor
	ShowCursor(FALSE);

#ifdef _DEBUG
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	ComPtr<IDXGIFactory4> pFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory)));

	// Attempt to create a hardware based device first.  If that fails, 
	// then fallback to WARP/software.
	HRESULT hardware_driver = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&mDevice)
		);

	if (!SUCCEEDED(hardware_driver))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(pFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&mDevice)
			));
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// Describe the swap chain.
	DXGI_SWAP_CHAIN_DESC descSwapChain = {};
	descSwapChain.BufferCount = 2;
	descSwapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	descSwapChain.OutputWindow = hWnd;
	descSwapChain.SampleDesc.Count = 1;
	descSwapChain.Windowed = TRUE;

	ThrowIfFailed(pFactory->CreateSwapChain(
		mCommandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		&descSwapChain,
		&mSwapChain
		));


	// root signature here
	// do not use a root signature ... we do not have shaders here 

	// PSO here 
	// do not use a pipeline state object ... we do not have shaders
	mPSO = nullptr;


	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
	descHeap.NumDescriptors = 1;
	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&mDescriptorHeap)));

	// Create render target view (RTV).
	ThrowIfFailed(mSwapChain->GetBuffer(0, IID_PPV_ARGS(&mRenderTarget)));
	mDevice->CreateRenderTargetView(mRenderTarget.Get(), nullptr, mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	// allocate memory for a command list and create one
	ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
	ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), mPSO.Get(), IID_PPV_ARGS(&mCommandList)));


	// Create and initialize the fence.
	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	mCurrentFence = 1;

	ThrowIfFailed(mCommandList->Close());

	// Create an event handle to use for frame synchronization.
	mHandleEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

	// setup timer 
	StartTime = GetTickCount();
	CurrentTime = 0;

	// set the game loop to running by default
	BRunning = TRUE;
	MSG msg;

	while (BRunning)
	{
		// Just remove the message
		PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE);

		// Calculate the current demo time
		CurrentTime = GetTickCount() - StartTime;

		// go out of game loop and shutdown
		if (CurrentTime > 3300 || GetAsyncKeyState(VK_ESCAPE))
			BRunning = FALSE;



		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(mCommandAllocator->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), mPSO.Get()));

		// Indicate that the back buffer will be used as a render target.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Record commands.
		float clearColor [] = { 0.0f, 0.2f, 0.4f, 1.0f };
		mCommandList->ClearRenderTargetView(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), clearColor, 0, nullptr);

		// Indicate that the back buffer will now be used to present.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		ThrowIfFailed(mCommandList->Close());

		// Execute the command list.
		ID3D12CommandList* ppCommandLists [] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present and move to the next back buffer.
		ThrowIfFailed(mSwapChain->Present(1, 0));
		mIndexLastSwapBuf = (1 + mIndexLastSwapBuf) % cNumSwapBufs;
		mSwapChain->GetBuffer(mIndexLastSwapBuf, IID_PPV_ARGS(&mRenderTarget));
		mDevice->CreateRenderTargetView(mRenderTarget.Get(), nullptr, mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		WaitForPreviousFrame();
	}


	// Wait for the GPU to be done with all resources.
	WaitForPreviousFrame();

	CloseHandle(mHandleEvent);

	return (int) msg.wParam;
}