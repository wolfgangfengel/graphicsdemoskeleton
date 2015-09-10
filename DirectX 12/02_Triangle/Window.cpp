////////////////////////////////////////////////////////////////////////
//
// Skeleton Intro Coding
//
// by Wolfgang Engel 
// Last time modified: 09/03/2015
//
///////////////////////////////////////////////////////////////////////

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_4.h>

// shaders as headers 
#include "vertex.sh"
#include "pixel.sh"


// define the size of the window
#define WINWIDTH 800 
#define WINHEIGHT 600
#define WINPOSX 200 
#define WINPOSY 200

int mIndexLastSwapBuf = 0;
int cNumSwapBufs = 2;

struct Vertex
{
	float position[3];
	float color[4];
};

// Pipeline objects.
IDXGISwapChain* mSwapChain;
ID3D12Device* mDevice;
ID3D12Resource* mRenderTarget;
ID3D12CommandAllocator* mCommandAllocator;
ID3D12CommandQueue* mCommandQueue;
ID3D12DescriptorHeap* mDescriptorHeap;
ID3D12PipelineState* mPSO;
ID3D12GraphicsCommandList* mCommandList;
ID3D12RootSignature* mRootSignature;
D3D12_VIEWPORT mViewport;
D3D12_RECT mRectScissor;


// App resources.
ID3D12Resource* mBufVerts;
D3D12_VERTEX_BUFFER_VIEW mDescViewBufVert;


// Synchronization objects.
HANDLE mHandleEvent;
ID3D12Fence* mFence;
UINT64 mCurrentFence;

EXTERN_C int _fltused = 0; // to get rid of the unresolved symbol __ftlused error

#if _DEBUG
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}
#else
inline void ThrowIfFailed(HRESULT hr){}
#endif

// to avoid libc
void *memcpy(void *v_dst, const void *v_src, unsigned int c)
{
	const char *src = (const char *)v_src;
	char *dst = (char *)v_dst;

	/* Simple, byte oriented memcpy. */
	while (c--)
		*dst++ = *src++;

	return v_dst;
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


#if _DEBUG
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
	descSwapChain.BufferCount = 2;
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


	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC layout [] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = _countof(layout);

	// Create an empty root signature.
	{
		ID3DBlob* pOutBlob;
		ID3DBlob* pErrorBlob;
		D3D12_ROOT_SIGNATURE_DESC descRootSignature;
		descRootSignature = { 0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
		ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
		ThrowIfFailed(mDevice->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
	}


	// rasterizer state
	static D3D12_RASTERIZER_DESC rasterizer =
	{
		D3D12_FILL_MODE_SOLID,
		D3D12_CULL_MODE_BACK,
		false,
		D3D12_DEFAULT_DEPTH_BIAS,
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		true,
		false,
		false,
		0,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
	};

	// this includes the D3D12_RENDER_TARGET_BLEND_DESC
	static D3D12_BLEND_DESC blendstate = { false, false, {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	} };


	// Describe and create a graphics pipeline state object (PSO).
	static D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso; 
	descPso.InputLayout = { layout, numElements };
	descPso.pRootSignature = mRootSignature;
	descPso.VS = { g_VShader, sizeof(g_VShader) };
	descPso.PS = { g_PShader, sizeof(g_PShader) };
	descPso.RasterizerState = rasterizer;
	descPso.BlendState = blendstate;
	descPso.DepthStencilState.DepthEnable = FALSE;
	descPso.DepthStencilState.StencilEnable = FALSE;
	descPso.SampleMask = UINT_MAX;
	descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	descPso.NumRenderTargets = 1;
	descPso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	descPso.SampleDesc.Count = 1;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&descPso, IID_PPV_ARGS(&mPSO)));


	// Describe and create a render target view (RTV) descriptor heap.
	static D3D12_DESCRIPTOR_HEAP_DESC descHeap; 
	descHeap.NumDescriptors = 1;
	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&mDescriptorHeap)));

	// Create render target view (RTV).
	ThrowIfFailed(mSwapChain->GetBuffer(0, IID_PPV_ARGS(&mRenderTarget)));
	mDevice->CreateRenderTargetView(mRenderTarget, nullptr, mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	// allocate memory for a command list and create one
	ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
	ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, mPSO, IID_PPV_ARGS(&mCommandList)));


	mViewport = { 0.0f, 0.0f, static_cast<float>(WINWIDTH), static_cast<float>(WINHEIGHT), 0.0f, 1.0f };

	mRectScissor = { 0, 0, static_cast<LONG>(WINWIDTH), static_cast<LONG>(WINHEIGHT) };

	// Define the geometry for a triangle.
	Vertex triangleVerts [] =
	{
		{ { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.45f, -0.5, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.45f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	// Create the vertex buffer.
	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	static D3D12_HEAP_PROPERTIES heapProperties = 
	{ 
		D3D12_HEAP_TYPE_UPLOAD, 
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN, 
		D3D12_MEMORY_POOL_UNKNOWN, 
		1, 
		1 
	};

	static D3D12_RESOURCE_DESC VertexBufferDesc = 
	{ 
		D3D12_RESOURCE_DIMENSION_BUFFER,			// type
		0,											// alignment
		_countof(triangleVerts) * sizeof(Vertex),	// size in bytes
		1,											// height 
		1,											// depthOrArraySize
		1,											// mip levels
		DXGI_FORMAT_UNKNOWN,						// format
		{ 1, 0 },									// sample description
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,				// layout
		D3D12_RESOURCE_FLAG_NONE					// flags
	}; 

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&VertexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,    // Clear value
		IID_PPV_ARGS(&mBufVerts)));


	// Copy the triangle data to the vertex buffer.
	UINT8* dataBegin;
	mBufVerts->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin));
	memcpy(dataBegin, triangleVerts, sizeof(triangleVerts));
	//dataBegin = triangleVerts
	mBufVerts->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	mDescViewBufVert.BufferLocation = mBufVerts->GetGPUVirtualAddress();
	mDescViewBufVert.StrideInBytes = sizeof(Vertex);
	mDescViewBufVert.SizeInBytes = sizeof(triangleVerts);


	// Create and initialize the fence.
	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	mCurrentFence = 1;

	// Close the command list and use it to execute the initial GPU setup.
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
		ThrowIfFailed(mCommandAllocator->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(mCommandList->Reset(mCommandAllocator, mPSO));

		mCommandList->SetGraphicsRootSignature(mRootSignature);
		mCommandList->RSSetViewports(1, &mViewport);

		// this is now a requirement to use, although we set the viewport above
		mCommandList->RSSetScissorRects(1, &mRectScissor);


		// Indicate that the back buffer will be used as a render target.
		const D3D12_RESOURCE_BARRIER barrierRTAsTexture = 
		{
			D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			D3D12_RESOURCE_BARRIER_FLAG_NONE,
			{ mRenderTarget, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET }
		};

		mCommandList->ResourceBarrier(1, &barrierRTAsTexture);

		// Record commands.
		float clearColor [] = { 0.0f, 0.2f, 0.4f, 1.0f };
		mCommandList->ClearRenderTargetView(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), clearColor, 0, nullptr);
		mCommandList->OMSetRenderTargets(1, &mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), true, nullptr);
		mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mCommandList->IASetVertexBuffers(0, 1, &mDescViewBufVert);
		mCommandList->DrawInstanced(3, 1, 0, 0);


		// Indicate that the back buffer will now be used to present.
		const D3D12_RESOURCE_BARRIER barrierRTForPresent =
		{ 
			D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, 
			D3D12_RESOURCE_BARRIER_FLAG_NONE, 
			{ mRenderTarget, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT }
		};

		mCommandList->ResourceBarrier(1, &barrierRTForPresent);

		ThrowIfFailed(mCommandList->Close());


		// Execute the command list.
		ID3D12CommandList* ppCommandLists [] = { mCommandList };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present and move to the next back buffer.
		ThrowIfFailed(mSwapChain->Present(1, 0));
		mIndexLastSwapBuf = (1 + mIndexLastSwapBuf) % cNumSwapBufs;
		mSwapChain->GetBuffer(mIndexLastSwapBuf, IID_PPV_ARGS(&mRenderTarget));
		mDevice->CreateRenderTargetView(mRenderTarget, nullptr, mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		WaitForPreviousFrame();
	}


	// Wait for the GPU to be done with all resources.
	WaitForPreviousFrame();

#if defined(WELLBEHAVIOUR)
	// I think I need to release a lot of stuff here ..
	mDevice->Release() ;
	mSwapChain->Release();
	mRenderTarget->Release();
	mCommandAllocator->Release();
	mCommandQueue->Release();
	mDescriptorHeap->Release();
	mPSO->Release();
	mCommandList->Release();
	mRootSignature->Release();
	mBufVerts->Release();
	mFence->Release();
#endif

#if _DEBUG
//	if (debugController) debugController->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
//	debugController->Release();
#endif

#if defined(REGULARENTRYPOINT)
	return (int) msg.wParam;
#else
	}
	ExitProcess(0);
#endif
}