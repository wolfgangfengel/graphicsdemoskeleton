////////////////////////////////////////////////////////////////////////
//
// Skeleton Intro Coding
//
// by Wolfgang Engel 
// Last time modified: 09/22/2011 (started sometime in 2003 or maybe much longer ago)
//
///////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <Windows.h>
#include <sal.h>
#include <rpcsal.h>





#define DEFINE_GUIDW(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
//DEFINE_GUIDW(IID_ID3D11Texture2D, 0x6f15aaf2, 0xd208, 0x4e89, 0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c);

DEFINE_GUIDW(IID_ID3D12Object, 0xc4fec28f, 0x7966, 0x4e95, 0x9f, 0x94, 0xf4, 0x31, 0xcb, 0x56, 0xc3, 0xb8);
DEFINE_GUIDW(IID_ID3D12DeviceChild, 0x905db94b, 0xa00c, 0x4140, 0x9d, 0xf5, 0x2b, 0x64, 0xca, 0x9e, 0xa3, 0x57);
DEFINE_GUIDW(IID_ID3D12RootSignature, 0xc54a6b66, 0x72df, 0x4ee8, 0x8b, 0xe5, 0xa9, 0x46, 0xa1, 0x42, 0x92, 0x14);
DEFINE_GUIDW(IID_ID3D12RootSignatureDeserializer, 0x34AB647B, 0x3CC8, 0x46AC, 0x84, 0x1B, 0xC0, 0x96, 0x56, 0x45, 0xC0, 0x46);
DEFINE_GUIDW(IID_ID3D12Pageable, 0x63ee58fb, 0x1268, 0x4835, 0x86, 0xda, 0xf0, 0x08, 0xce, 0x62, 0xf0, 0xd6);
DEFINE_GUIDW(IID_ID3D12Heap, 0x6b3b2502, 0x6e51, 0x45b3, 0x90, 0xee, 0x98, 0x84, 0x26, 0x5e, 0x8d, 0xf3);
DEFINE_GUIDW(IID_ID3D12Resource, 0x696442be, 0xa72e, 0x4059, 0xbc, 0x79, 0x5b, 0x5c, 0x98, 0x04, 0x0f, 0xad);
DEFINE_GUIDW(IID_ID3D12CommandAllocator, 0x6102dee4, 0xaf59, 0x4b09, 0xb9, 0x99, 0xb4, 0x4d, 0x73, 0xf0, 0x9b, 0x24);
DEFINE_GUIDW(IID_ID3D12Fence, 0x0a753dcf, 0xc4d8, 0x4b91, 0xad, 0xf6, 0xbe, 0x5a, 0x60, 0xd9, 0x5a, 0x76);
DEFINE_GUIDW(IID_ID3D12PipelineState, 0x765a30f3, 0xf624, 0x4c6f, 0xa8, 0x28, 0xac, 0xe9, 0x48, 0x62, 0x24, 0x45);
//DEFINE_GUIDW(IID_ID3D12DescriptorHeap,0x8efb471d,0x616c,0x4f49,0x90,0xf7,0x12,0x7b,0xb7,0x63,0xfa,0x51);
DEFINE_GUIDW(IID_ID3D12DescriptorHeap, 0x8efb471d, 0x616c, 0x4f49, 0x90, 0xf7, 0x12, 0x7b, 0xb7, 0x63, 0xfa, 0x51);
DEFINE_GUIDW(IID_ID3D12QueryHeap, 0x0d9658ae, 0xed45, 0x469e, 0xa6, 0x1d, 0x97, 0x0e, 0xc5, 0x83, 0xca, 0xb4);
DEFINE_GUIDW(IID_ID3D12CommandSignature, 0xc36a797c, 0xec80, 0x4f0a, 0x89, 0x85, 0xa7, 0xb2, 0x47, 0x50, 0x82, 0xd1);
DEFINE_GUIDW(IID_ID3D12CommandList, 0x7116d91c, 0xe7e4, 0x47ce, 0xb8, 0xc6, 0xec, 0x81, 0x68, 0xf4, 0x37, 0xe5);
DEFINE_GUIDW(IID_ID3D12GraphicsCommandList, 0x5b160d0f, 0xac1b, 0x4185, 0x8b, 0xa8, 0xb3, 0xae, 0x42, 0xa5, 0xa4, 0x55);
DEFINE_GUIDW(IID_ID3D12CommandQueue, 0x0ec870a6, 0x5d7e, 0x4c22, 0x8c, 0xfc, 0x5b, 0xaa, 0xe0, 0x76, 0x16, 0xed);
DEFINE_GUIDW(IID_ID3D12Device, 0x189819f1, 0x1db6, 0x4b57, 0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7);


// d3d12sdklayers.h
DEFINE_GUIDW(DXGI_DEBUG_D3D12, 0xcf59a98c, 0xa950, 0x4326, 0x91, 0xef, 0x9b, 0xba, 0xa1, 0x7b, 0xfd, 0x95);

DEFINE_GUIDW(IID_ID3D12Debug, 0x344488b7, 0x6846, 0x474b, 0xb9, 0x89, 0xf0, 0x27, 0x44, 0x82, 0x45, 0xe0);
DEFINE_GUIDW(IID_ID3D12DebugDevice, 0x3febd6dd, 0x4973, 0x4787, 0x81, 0x94, 0xe4, 0x5f, 0x9e, 0x28, 0x92, 0x3e);
DEFINE_GUIDW(IID_ID3D12DebugCommandQueue, 0x09e0bf36, 0x54ac, 0x484f, 0x88, 0x47, 0x4b, 0xae, 0xea, 0xb6, 0x05, 0x3a);
DEFINE_GUIDW(IID_ID3D12DebugCommandList, 0x09e0bf36, 0x54ac, 0x484f, 0x88, 0x47, 0x4b, 0xae, 0xea, 0xb6, 0x05, 0x3f);
DEFINE_GUIDW(IID_ID3D12InfoQueue, 0x0742a90b, 0xc387, 0x483f, 0xb9, 0x46, 0x30, 0xa7, 0xe4, 0xe6, 0x14, 0x58);

/*
// dxgi.h 
DEFINE_GUIDW(IID_IDXGIObject, 0xaec22fb8, 0x76f3, 0x4639, 0x9b, 0xe0, 0x28, 0xeb, 0x43, 0xa6, 0x7a, 0x2e);
DEFINE_GUIDW(IID_IDXGIDeviceSubObject, 0x3d3e0379, 0xf9de, 0x4d58, 0xbb, 0x6c, 0x18, 0xd6, 0x29, 0x92, 0xf1, 0xa6);
DEFINE_GUIDW(IID_IDXGIResource, 0x035f3ab4, 0x482e, 0x4e50, 0xb4, 0x1f, 0x8a, 0x7f, 0x8b, 0xd8, 0x96, 0x0b);
DEFINE_GUIDW(IID_IDXGIKeyedMutex, 0x9d8e1289, 0xd7b3, 0x465f, 0x81, 0x26, 0x25, 0x0e, 0x34, 0x9a, 0xf8, 0x5d);
DEFINE_GUIDW(IID_IDXGISurface, 0xcafcb56c, 0x6ac3, 0x4889, 0xbf, 0x47, 0x9e, 0x23, 0xbb, 0xd2, 0x60, 0xec);
DEFINE_GUIDW(IID_IDXGISurface1, 0x4AE63092, 0x6327, 0x4c1b, 0x80, 0xAE, 0xBF, 0xE1, 0x2E, 0xA3, 0x2B, 0x86);
DEFINE_GUIDW(IID_IDXGIAdapter, 0x2411e7e1, 0x12ac, 0x4ccf, 0xbd, 0x14, 0x97, 0x98, 0xe8, 0x53, 0x4d, 0xc0);
DEFINE_GUIDW(IID_IDXGIOutput, 0xae02eedb, 0xc735, 0x4690, 0x8d, 0x52, 0x5a, 0x8d, 0xc2, 0x02, 0x13, 0xaa);
DEFINE_GUIDW(IID_IDXGISwapChain, 0x310d36a0, 0xd2e7, 0x4c0a, 0xaa, 0x04, 0x6a, 0x9d, 0x23, 0xb8, 0x88, 0x6a);
DEFINE_GUIDW(IID_IDXGIFactory, 0x7b7166ec, 0x21c7, 0x44ae, 0xb2, 0x1a, 0xc9, 0xae, 0x32, 0x1a, 0xe3, 0x69);
DEFINE_GUIDW(IID_IDXGIDevice, 0x54ec77fa, 0x1377, 0x44e6, 0x8c, 0x32, 0x88, 0xfd, 0x5f, 0x44, 0xc8, 0x4c);
DEFINE_GUIDW(IID_IDXGIFactory1, 0x770aae78, 0xf26f, 0x4dba, 0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87);
DEFINE_GUIDW(IID_IDXGIAdapter1, 0x29038f61, 0x3839, 0x4626, 0x91, 0xfd, 0x08, 0x68, 0x79, 0x01, 0x1a, 0x05);
DEFINE_GUIDW(IID_IDXGIDevice1, 0x77db970f, 0x6276, 0x48ba, 0xba, 0x28, 0x07, 0x01, 0x43, 0xb4, 0x39, 0x2c);
*/

// dxgi1_4.h
DEFINE_GUIDW(IID_IDXGISwapChain3, 0x94d99bdb, 0xf1f8, 0x4ab0, 0xb2, 0x36, 0x7d, 0xa0, 0x17, 0x0e, 0xda, 0xb1);
DEFINE_GUIDW(IID_IDXGIOutput4, 0xdc7dca35, 0x2196, 0x414d, 0x9F, 0x53, 0x61, 0x78, 0x84, 0x03, 0x2a, 0x60);
DEFINE_GUIDW(IID_IDXGIFactory4, 0x1bc6ea02, 0xef36, 0x464f, 0xbf, 0x0c, 0x21, 0xca, 0x39, 0xe5, 0x16, 0x8a);
DEFINE_GUIDW(IID_IDXGIAdapter3, 0x645967A4, 0x1392, 0x4310, 0xA7, 0x98, 0x80, 0x53, 0xCE, 0x3E, 0x93, 0xFD);

#include <d3d12.h>
#include <dxgi1_4.h>



// define the size of the window
#define WINWIDTH 800 
#define WINHEIGHT 600
#define WINPOSX 200 
#define WINPOSY 200

#define FRAMECOUNT 2

// Pipeline objects.
IDXGISwapChain3* mSwapChain;
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
	ThrowIfFailed(mCommandQueue->lpVtbl->Signal(mCommandQueue, mFence, fence));
	mCurrentFence++;

	// Wait until the previous frame is finished.
	if (mFence->lpVtbl->GetCompletedValue(mFence) < fence)
	{
		ThrowIfFailed(mFence->lpVtbl->SetEventOnCompletion(mFence, fence, mHandleEvent));
		WaitForSingleObject(mHandleEvent, INFINITE);
	}

//	mframeIndex = mSwapChain->lpVtbl->GetCurrentBackBufferIndex(mSwapChain);
}


D3D12_CPU_DESCRIPTOR_HANDLE OffsetDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, INT offsetInDescriptors, UINT descriptorIncrementSize)
{
	handle.ptr += offsetInDescriptors * descriptorIncrementSize;
	return handle;
}


// makes the applicaton behave well with windows
// allows to remove some system calls to reduce size
#define WELLBEHAVIOUR

// for demos we can use an entry point that occupies less "space" then the regular entry point
// if you change back to the regualar entry point you need to remove winmain in Visual Studio 2013 under Linker -> Advanced Entrypoint 
//#define REGULARENTRYPOINT

#if defined(REGULARENTRYPOINT)


// this is a simplified entry point ...
void __stdcall WinMainCRTStartup()
{
    ExitProcess(WinMain(GetModuleHandle(NULL), NULL, NULL, 0));
}

// this is the main windows entry point ... 
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#else
// Take away prolog and epilog, then put a minial prolog back manually with assembly below. The function never returns so no epilog is necessary.
__declspec( naked )  void __cdecl winmain()

{
	// Prolog
	//__asm enter 0x10, 0;
	__asm 
	{
		push ebp
        mov ebp,esp
        sub esp,__LOCAL_SIZE
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


	// Load the pipeline
#ifdef _DEBUG
	// Enable the D3D12 debug layer.
	{
		ID3D12Debug* debugController = NULL;
		D3D12GetDebugInterface((REFIID) &IID_ID3D12Debug, (LPVOID*) (&debugController));
		debugController->lpVtbl->EnableDebugLayer(debugController);
	}
#endif

	IDXGIFactory4* pFactory;
	ThrowIfFailed(CreateDXGIFactory1((REFIID)&IID_IDXGIFactory4, (LPVOID*)(&pFactory)));

	// Attempt to create a hardware based device first.  If that fails, 
	// then fallback to WARP/software.
	HRESULT hardware_driver = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, (REFIID)&IID_ID3D12Device, (LPVOID*)(&mDevice));


#ifdef _DEBUG
	if (!SUCCEEDED(hardware_driver))
	{
		IDXGIAdapter* pWarpAdapter;
		ThrowIfFailed(pFactory->lpVtbl->EnumWarpAdapter(pFactory, (REFIID)&IID_IDXGIAdapter3, (LPVOID*)(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter,
			D3D_FEATURE_LEVEL_11_0,
			(REFIID)&IID_ID3D12Device, (LPVOID*)(&mDevice)
			));
	}
#endif

	static D3D12_COMMAND_QUEUE_DESC queueDesc;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(mDevice->lpVtbl->CreateCommandQueue(mDevice, &queueDesc, (REFIID)&IID_ID3D12CommandQueue, (LPVOID*)(&mCommandQueue)));

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
	
	pFactory->lpVtbl->CreateSwapChain(pFactory, mCommandQueue,		// Swap chain needs the queue so that it can force a flush on it.
									  &descSwapChain,
									  &mSwapChain);

//	mframeIndex = mSwapChain->lpVtbl->GetCurrentBackBufferIndex(mSwapChain);


	// root signature here
	// do not use a root signature ... we do not have shaders here 

	// PSO here 
	// do not use a pipeline state object ... we do not have shaders
	mPSO = NULL;

	// Describe and create a render target view (RTV) descriptor heap.
	static D3D12_DESCRIPTOR_HEAP_DESC descHeap;
	descHeap.NumDescriptors = FRAMECOUNT;
	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // RTV is not shader visible
	ThrowIfFailed(mDevice->lpVtbl->CreateDescriptorHeap(mDevice, &descHeap, (REFIID)&IID_ID3D12DescriptorHeap, (LPVOID*)(&mDescriptorHeap)));

	mrtvDescriptorIncrSize = mDevice->lpVtbl->GetDescriptorHandleIncrementSize(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

#ifdef _DEBUG
	static D3D12_DESCRIPTOR_HEAP_DESC debugdescHeap;
	debugdescHeap = mDescriptorHeap->lpVtbl->GetDesc(mDescriptorHeap);
#endif

	// Create frame resources
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	rtvHandle = mDescriptorHeap->lpVtbl->GetCPUDescriptorHandleForHeapStart(mDescriptorHeap);

	// Create a RTV for each frame.
	for (UINT n = 0; n < FRAMECOUNT; n++)
	{
		ThrowIfFailed(mSwapChain->lpVtbl->GetBuffer(mSwapChain, n, (REFIID)&IID_ID3D12Resource, (LPVOID*)(&mRenderTarget[n]))); //IID_PPV_ARGS(&m_renderTargets[n])));
		mDevice->lpVtbl->CreateRenderTargetView(mDevice, mRenderTarget[n], NULL, rtvHandle);
		rtvHandle = OffsetDescriptor(rtvHandle, 1, mrtvDescriptorIncrSize);
	}

	// allocate memory for a command list and create one
	mDevice->lpVtbl->CreateCommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, (REFIID)&IID_ID3D12CommandAllocator, (LPVOID*)(&mCommandAllocator));
	mDevice->lpVtbl->CreateCommandList(mDevice, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, mPSO, (REFIID)&IID_ID3D12CommandList, (LPVOID*)(&mCommandList));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(mCommandList->lpVtbl->Close(mCommandList));


	// Create and initialize the fence.
	ThrowIfFailed(mDevice->lpVtbl->CreateFence(mDevice, 0, D3D12_FENCE_FLAG_NONE, (REFIID)&IID_ID3D12Fence, (LPVOID*)(&mFence)));
	mCurrentFence = 1;


	// Create an event handle to use for frame synchronization.
	mHandleEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);

	// setup timer 
	StartTime = GetTickCount();
	CurrentTime = 0;

	// set the game loop to running by default
	BRunning = TRUE;
	MSG msg;

	mframeIndex = 0;

	while (BRunning)
	{
#if defined(WELLBEHAVIOUR)
		// Just remove the message
		PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE);
#endif
		// Calculate the current demo time
		CurrentTime = GetTickCount() - StartTime;

		// go out of game loop and shutdown
		if (CurrentTime > 3300 || GetAsyncKeyState(VK_ESCAPE)) 
			BRunning = FALSE;

		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(mCommandAllocator->lpVtbl->Reset(mCommandAllocator));

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(mCommandList->lpVtbl->Reset(mCommandList, mCommandAllocator, mPSO));

		mframeIndex++;
		mframeIndex = (mframeIndex >= 1) ? 0 : mframeIndex;

		// Indicate that the back buffer will be used as a render target.
		const D3D12_RESOURCE_BARRIER barrierRTAsTexture =
		{
			D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			D3D12_RESOURCE_BARRIER_FLAG_NONE,
			{ mRenderTarget[mframeIndex], D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET }
		};

		mCommandList->lpVtbl->ResourceBarrier(mCommandList, 1, &barrierRTAsTexture);

		rtvHandle = mDescriptorHeap->lpVtbl->GetCPUDescriptorHandleForHeapStart(mDescriptorHeap);
		rtvHandle = OffsetDescriptor(rtvHandle, mframeIndex, mrtvDescriptorIncrSize);

		// Record commands.
		float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		mCommandList->lpVtbl->ClearRenderTargetView(mCommandList, rtvHandle, clearColor, 0, NULL);

		// Indicate that the back buffer will now be used to present.
		const D3D12_RESOURCE_BARRIER barrierRTForPresent =
		{
			D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			D3D12_RESOURCE_BARRIER_FLAG_NONE,
			{ mRenderTarget[mframeIndex], D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT }
		};

		mCommandList->lpVtbl->ResourceBarrier(mCommandList, 1, &barrierRTForPresent);

		ThrowIfFailed(mCommandList->lpVtbl->Close(mCommandList));

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { mCommandList };
		mCommandQueue->lpVtbl->ExecuteCommandLists(mCommandQueue, _countof(ppCommandLists), ppCommandLists);

		// Present and move to the next back buffer.
		ThrowIfFailed(mSwapChain->lpVtbl->Present(mSwapChain, 1, 0));
		mframeIndex = (1 + mframeIndex) % FRAMECOUNT;

		WaitForPreviousFrame();
	}


	// Wait for the GPU to be done with all resources.
	WaitForPreviousFrame();

#if defined(WELLBEHAVIOUR)
	// I think I need to release a lot of stuff here ..
	mDevice->lpVtbl->Release(mDevice);
	mSwapChain->lpVtbl->Release(mSwapChain);
	// Create a RTV for each frame.
	for (UINT n = 0; n < FRAMECOUNT; n++)
	{
		mRenderTarget[n]->lpVtbl->Release(mRenderTarget[n]);
	}
	mCommandAllocator->lpVtbl->Release(mCommandAllocator);
	mCommandQueue->lpVtbl->Release(mCommandQueue);
	mDescriptorHeap->lpVtbl->Release(mDescriptorHeap);
	mCommandList->lpVtbl->Release(mCommandList);
	//		mPSO->Release();
	mFence->lpVtbl->Release(mFence);
#endif

#if defined(REGULARENTRYPOINT)
    return (int) msg.wParam;
#else
	}

	ExitProcess(0);
#endif
}
