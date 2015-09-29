////////////////////////////////////////////////////////////////////////
//
// Skeleton Intro Coding
//
// by Wolfgang Engel 
// Last time modified: 09/10/2015
//
///////////////////////////////////////////////////////////////////////

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_4.h>


// define the size of the window
#define WINWIDTH 800 
#define WINHEIGHT 600
#define WINPOSX 200 
#define WINPOSY 200

#define FRAMECOUNT 2


// Pipeline objects.
IDXGISwapChain* mSwapChain;
ID3D12Device* mDevice;
ID3D12Resource* mRenderTarget[FRAMECOUNT];
ID3D12CommandAllocator* mCommandAllocator;
ID3D12CommandQueue* mCommandQueue;
ID3D12DescriptorHeap* mDescriptorHeap;
ID3D12PipelineState* mPSO;
ID3D12GraphicsCommandList* mCommandList;

// Synchronization objects.
HANDLE mHandleEvent;
ID3D12Fence* mFence;
UINT64 mCurrentFence;

UINT mrtvDescriptorIncrSize;
UINT mframeIndex;

EXTERN_C int _fltused = 0; // to get rid of the unresolved symbol __ftlused error


inline void ThrowIfFailed(HRESULT hr)
{
#if defined(_DEBUG)
	if (hr != S_OK)
		MessageBoxA(NULL, "Function call failed", "Error", MB_OK | MB_ICONERROR);
#endif
}


void WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. More advanced samples 
	// illustrate how to use fences for efficient resource usage.

	// Signal and increment the fence value.
	const UINT64 fence = mCurrentFence;
	ThrowIfFailed(mCommandQueue->Signal(mFence, fence));
	mCurrentFence++;

	// Wait until the previous frame is finished.
	if (mFence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(fence, mHandleEvent));
		WaitForSingleObject(mHandleEvent, INFINITE);
	}

	// would require swap chain 3 interface
//	mframeIndex = mSwapChain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE OffsetDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, INT offsetInDescriptors, UINT descriptorIncrementSize)
{
	handle.ptr += offsetInDescriptors * descriptorIncrementSize;
	return handle;
}

// allows to remove some system calls to reduce size -> application doesn't comply with windows standard behaviour anymore, so be careful
#define WELLBEHAVIOUR

// for demos we can use an entry point that occupies less "space" then the regular entry point
// if you change back to the regualar entry point you need to remove winmain in Visual Studio 2013 under Linker -> Advanced Entrypoint 
//#define REGULARENTRYPOINT

#if defined(REGULARENTRYPOINT)

// this is the main windows entry point ... 
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#else
// Take away prolog and epilog, then put a minial prolog back manually with assembly below. The function never returns so no epilog is necessary.
__declspec(naked)  void __cdecl winmain()

{
	// Prolog
	//__asm enter 0x10, 0;
	__asm
	{
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE
	}

	{ // Extra scope to make compiler accept the __decalspec(naked) with local variables

#endif

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
		ID3D12Debug* debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	IDXGIFactory4* pFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory)));

	// Attempt to create a hardware based device first.  If that fails, 
	// then fallback to WARP/software.
	HRESULT hardware_driver = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&mDevice)
		);


#ifdef _DEBUG
	if (!SUCCEEDED(hardware_driver))
	{
		IDXGIAdapter* pWarpAdapter;
		ThrowIfFailed(pFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&mDevice)
 			));
	}
#endif


		static D3D12_COMMAND_QUEUE_DESC queueDesc;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

		// Describe the swap chain.
		static DXGI_SWAP_CHAIN_DESC descSwapChain;
		descSwapChain.BufferCount = FRAMECOUNT;
		descSwapChain.BufferDesc.Width = WINWIDTH;
		descSwapChain.BufferDesc.Height = WINHEIGHT;
		descSwapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		descSwapChain.OutputWindow = hWnd;
		descSwapChain.SampleDesc.Count = 1;
		descSwapChain.Windowed = TRUE;

		ThrowIfFailed(pFactory->CreateSwapChain(
			mCommandQueue,		// Swap chain needs the queue so that it can force a flush on it.
			&descSwapChain,
			&mSwapChain
			));

//		mframeIndex = mSwapChain->GetCurrentBackBufferIndex();


		// root signature here
		// do not use a root signature ... we do not have shaders here 

		// PSO here 
		// do not use a pipeline state object ... we do not have shaders
		mPSO = nullptr;


		// Describe and create a render target view (RTV) descriptor heap.
		static D3D12_DESCRIPTOR_HEAP_DESC descHeap ;
		descHeap.NumDescriptors = FRAMECOUNT;
		descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(mDevice->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&mDescriptorHeap)));

		mrtvDescriptorIncrSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


		// Create render target view (RTV).
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
		rtvHandle = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		// Create a RTV for each frame.
		for (UINT n = 0; n < FRAMECOUNT; n++)
		{
			ThrowIfFailed(mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTarget[n])));
			mDevice->CreateRenderTargetView(mRenderTarget[n], nullptr, rtvHandle);
			rtvHandle = OffsetDescriptor(rtvHandle, 1, mrtvDescriptorIncrSize);
//			rtvHandle.Offset(1, mrtvDescriptorIncrSize);
		}

		// allocate memory for a command list and create one
		ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
		ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, mPSO, IID_PPV_ARGS(&mCommandList)));

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

		mframeIndex = 0;
		
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
			ThrowIfFailed(mCommandList->Reset(mCommandAllocator, mPSO));

			// Indicate that the back buffer will be used as a render target.
			const D3D12_RESOURCE_BARRIER barrierRTAsTexture =
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAG_NONE,
				{ mRenderTarget[mframeIndex], D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET }
			};

			mCommandList->ResourceBarrier(1, &barrierRTAsTexture);

			rtvHandle = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			rtvHandle = OffsetDescriptor(rtvHandle, mframeIndex, mrtvDescriptorIncrSize);

			// Record commands.
			float clearColor [] = { 0.0f, 0.2f, 0.4f, 1.0f };
			mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

			// Indicate that the back buffer will now be used to present.
			const D3D12_RESOURCE_BARRIER barrierRTForPresent =
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAG_NONE,
				{ mRenderTarget[mframeIndex], D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT }
			};

			mCommandList->ResourceBarrier(1, &barrierRTForPresent);

			ThrowIfFailed(mCommandList->Close());

			// Execute the command list.
			ID3D12CommandList* ppCommandLists [] = { mCommandList };
			mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

			// Present and move to the next back buffer.
			ThrowIfFailed(mSwapChain->Present(1, 0));
			mframeIndex = (1 + mframeIndex) % FRAMECOUNT;

			WaitForPreviousFrame();
		}

		// Wait for the GPU to be done with all resources.
		WaitForPreviousFrame();

#if defined(WELLBEHAVIOUR)
		mDevice->Release();
		mSwapChain->Release();

		// Create a RTV for each frame.
		for (UINT n = 0; n < FRAMECOUNT; n++)
		{
			mRenderTarget[n]->Release();
		}

		mCommandAllocator->Release();
		mCommandQueue->Release();
		mDescriptorHeap->Release();
		mCommandList->Release();
//		mPSO->Release();
		mFence->Release();
#endif

#if defined(REGULARENTRYPOINT)
	return (int) msg.wParam;
#else
	}

	ExitProcess(0);
#endif
}