/* San Angeles Observation - DirectX 12, Win32, no C runtime.
 * Adapted from Jetro Lauha's 2004-2005 OpenGL ES demonstration.
 * Copyright (c) 2004-2005 Jetro Lauha. See ../../San Angeles/license-BSD.txt.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#define SA_EXTERNAL_VERTICES
#include "../../San Angeles/Scene.h"
#include "vertex.sh"
#include "pixel.sh"

#define WINWIDTH 1280
#define WINHEIGHT 720
#define FRAMECOUNT 2
#ifdef _DEBUG
#define WINDOW_TITLE L"San Angeles - DirectX 12"
#else
#define WINDOW_TITLE 0
#endif

static ID3D12Device *device;
static ID3D12CommandQueue *queue;
static ID3D12CommandAllocator *allocator;
static ID3D12GraphicsCommandList *commands;
static ID3D12RootSignature *rootSignature;
static ID3D12PipelineState *modelPipeline,*floorPipeline;
static ID3D12DescriptorHeap *rtvHeap,*dsvHeap;
static ID3D12Resource *vertices,*depth;
static ID3D12Fence *fence;
static UINT fenceValue; /* Fewer than 2^32 frames in the 109-second sequence. */
static D3D12_CPU_DESCRIPTOR_HANDLE depthView;
static D3D12_VERTEX_BUFFER_VIEW vertexView;
static SA_CONSTANTS constants;

static void check(HRESULT result) { if (FAILED(result)) ExitProcess((UINT)result); }

static D3D12_CPU_DESCRIPTOR_HANDLE heapStart(ID3D12DescriptorHeap *heap)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    /* The x86 C++ COM implementation returns this struct through a hidden
     * output pointer. The SDK C declaration needs this ABI workaround. */
    ((void (__stdcall *)(ID3D12DescriptorHeap *,D3D12_CPU_DESCRIPTOR_HANDLE *))
        heap->lpVtbl->GetCPUDescriptorHandleForHeapStart)(heap,&handle);
    return handle;
}

static void initializeRenderer(void)
{
    static const D3D12_INPUT_ELEMENT_DESC elements[]={
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };
    static const D3D12_COMMAND_QUEUE_DESC queueDesc={0};
    /* Initializers compress better than instructions that fill these fields.
     * Renderer initialization runs once per process. */
    static D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline={
        0,{g_VShader,sizeof(g_VShader)},{g_PShader,sizeof(g_PShader)},{0},{0},{0},{0},
        {FALSE,FALSE,{{FALSE,FALSE,D3D12_BLEND_ONE,D3D12_BLEND_ZERO,D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE,D3D12_BLEND_ZERO,D3D12_BLEND_OP_ADD,D3D12_LOGIC_OP_NOOP,D3D12_COLOR_WRITE_ENABLE_ALL}}},
        0xffffffffu,
        {D3D12_FILL_MODE_SOLID,D3D12_CULL_MODE_NONE,FALSE,0,0,0,TRUE,FALSE,FALSE,0,D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF},
        {TRUE,D3D12_DEPTH_WRITE_MASK_ALL,D3D12_COMPARISON_FUNC_LESS,FALSE,0,0,
            {D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS},
            {D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS}},
        {elements,3},D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        1,{DXGI_FORMAT_R8G8B8A8_UNORM},DXGI_FORMAT_D24_UNORM_S8_UINT,{1,0},0,{0},D3D12_PIPELINE_STATE_FLAG_NONE
    };
    static D3D12_DESCRIPTOR_HEAP_DESC heapDesc={D3D12_DESCRIPTOR_HEAP_TYPE_RTV,FRAMECOUNT,0,0};
    static D3D12_HEAP_PROPERTIES heap={D3D12_HEAP_TYPE_DEFAULT,0,0,1,1};
    static D3D12_RESOURCE_DESC resource={D3D12_RESOURCE_DIMENSION_TEXTURE2D,0,WINWIDTH,WINHEIGHT,1,1,DXGI_FORMAT_D24_UNORM_S8_UINT,{1,0},D3D12_TEXTURE_LAYOUT_UNKNOWN,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL};
    static D3D12_CLEAR_VALUE clear={0};
    static const D3D12_RANGE noRead={0,0};

    check(device->lpVtbl->CreateCommandQueue(device,&queueDesc,&IID_ID3D12CommandQueue,(void **)&queue));
    /* FXC embeds the 24-DWORD root signature in the vertex shader. Each draw
     * records its own constants; no runtime serialization or CB allocation. */
    check(device->lpVtbl->CreateRootSignature(device,0,g_VShader,sizeof(g_VShader),&IID_ID3D12RootSignature,(void **)&rootSignature));

    pipeline.pRootSignature=rootSignature;
    check(device->lpVtbl->CreateGraphicsPipelineState(device,&pipeline,&IID_ID3D12PipelineState,(void **)&modelPipeline));
    pipeline.DepthStencilState.DepthEnable=FALSE;
    pipeline.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
    pipeline.BlendState.RenderTarget[0].BlendEnable=TRUE;
    pipeline.BlendState.RenderTarget[0].SrcBlend=D3D12_BLEND_ZERO;
    pipeline.BlendState.RenderTarget[0].DestBlend=D3D12_BLEND_SRC_COLOR;
    pipeline.BlendState.RenderTarget[0].SrcBlendAlpha=D3D12_BLEND_ZERO;
    pipeline.BlendState.RenderTarget[0].DestBlendAlpha=D3D12_BLEND_ONE;
    check(device->lpVtbl->CreateGraphicsPipelineState(device,&pipeline,&IID_ID3D12PipelineState,(void **)&floorPipeline));

    check(device->lpVtbl->CreateDescriptorHeap(device,&heapDesc,&IID_ID3D12DescriptorHeap,(void **)&rtvHeap));
    heapDesc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV; heapDesc.NumDescriptors=1;
    check(device->lpVtbl->CreateDescriptorHeap(device,&heapDesc,&IID_ID3D12DescriptorHeap,(void **)&dsvHeap));
    depthView=heapStart(dsvHeap);
    clear.Format=resource.Format; clear.DepthStencil.Depth=1;
    check(device->lpVtbl->CreateCommittedResource(device,&heap,0,&resource,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,&IID_ID3D12Resource,(void **)&depth));
    device->lpVtbl->CreateDepthStencilView(device,depth,0,depthView);

    heap.Type=D3D12_HEAP_TYPE_UPLOAD;
    resource.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    resource.Width=SA_VERTEX_LIMIT*sizeof(SA_VERTEX); resource.Height=1;
    resource.Format=DXGI_FORMAT_UNKNOWN; resource.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR; resource.Flags=0;
    /* Reserve the generator's maximum capacity and keep the upload buffer
     * mapped. Finish all CPU writes before submitting any GPU work. */
    check(device->lpVtbl->CreateCommittedResource(device,&heap,0,&resource,D3D12_RESOURCE_STATE_GENERIC_READ,0,&IID_ID3D12Resource,(void **)&vertices));
    check(vertices->lpVtbl->Map(vertices,0,&noRead,(void **)&saVertices));
    saCreateScene();
    vertexView.BufferLocation=vertices->lpVtbl->GetGPUVirtualAddress(vertices);
    vertexView.StrideInBytes=sizeof(SA_VERTEX); vertexView.SizeInBytes=saVertexCount*sizeof(SA_VERTEX);
    check(device->lpVtbl->CreateCommandAllocator(device,D3D12_COMMAND_LIST_TYPE_DIRECT,&IID_ID3D12CommandAllocator,(void **)&allocator));
    check(device->lpVtbl->CreateCommandList(device,0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,modelPipeline,&IID_ID3D12GraphicsCommandList,(void **)&commands));
    check(commands->lpVtbl->Close(commands));
    check(device->lpVtbl->CreateFence(device,0,D3D12_FENCE_FLAG_NONE,&IID_ID3D12Fence,(void **)&fence));
    constants.right[3]=2.41421356237f*WINHEIGHT/WINWIDTH;
}

static void drawMesh(unsigned int mesh,float x,float y,float z,float scale,float angle,float mirror,float lit)
{
    constants.translateScale[0]=x; constants.translateScale[1]=y;
    constants.translateScale[2]=z; constants.translateScale[3]=scale;
    constants.rotateMirrorLight[0]=saCos(angle); constants.rotateMirrorLight[1]=saSin(angle);
    constants.rotateMirrorLight[2]=mirror; constants.rotateMirrorLight[3]=lit;
    commands->lpVtbl->SetGraphicsRoot32BitConstants(commands,0,sizeof(constants)/sizeof(float),&constants,0);
    commands->lpVtbl->DrawInstanced(commands,saMeshes[mesh].count,1,saMeshes[mesh].first,0);
}

static void drawModels(unsigned int tick,float mirror)
{
    int x,y;
    saSeed=9;
    for (y=-5;y<=5;++y) for (x=-5;x<=5;++x) {
        unsigned int shape=saRandom()%SUPERSHAPE_COUNT;
        float angle=(saRandom()%360)*(SA_PI/180);
        drawMesh(shape,(float)(x*9),(float)(y*9),0,sSuperShapeParams[shape][14],angle,mirror,1);
    }
    for (x=-2;x<=2;++x) {
        float offset=(x*4500+(int)(tick%4500))*0.01f;
        drawMesh(SUPERSHAPE_COUNT-1,offset,-4,2,1,0,mirror,1);
        drawMesh(SUPERSHAPE_COUNT-1,-4,offset,4,1,SA_PI/2,mirror,1);
    }
}

static void recordFrame(unsigned int tick,ID3D12Resource *target,D3D12_CPU_DESCRIPTOR_HANDLE view,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after)
{
    static const D3D12_VIEWPORT viewport={0,0,WINWIDTH,WINHEIGHT,0,1};
    static const D3D12_RECT scissor={0,0,WINWIDTH,WINHEIGHT};
    static D3D12_RESOURCE_BARRIER barrier={0};
    float clear[4];
    check(allocator->lpVtbl->Reset(allocator));
    check(commands->lpVtbl->Reset(commands,allocator,modelPipeline));
    barrier.Transition.pResource=target;
    barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore=before; barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;
    commands->lpVtbl->ResourceBarrier(commands,1,&barrier);
    commands->lpVtbl->SetGraphicsRootSignature(commands,rootSignature);
    commands->lpVtbl->RSSetViewports(commands,1,&viewport);
    commands->lpVtbl->RSSetScissorRects(commands,1,&scissor);
    commands->lpVtbl->OMSetRenderTargets(commands,1,&view,FALSE,&depthView);
    commands->lpVtbl->IASetPrimitiveTopology(commands,D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commands->lpVtbl->IASetVertexBuffers(commands,0,1,&vertexView);
    saCamera(tick,&constants);
    clear[0]=0.1f*constants.eye[3]; clear[1]=0.2f*constants.eye[3]; clear[2]=0.3f*constants.eye[3]; clear[3]=1;
    commands->lpVtbl->ClearRenderTargetView(commands,view,clear,0,0);
    commands->lpVtbl->ClearDepthStencilView(commands,depthView,D3D12_CLEAR_FLAG_DEPTH,1,0,0,0);
    drawModels(tick,-1);
    commands->lpVtbl->SetPipelineState(commands,floorPipeline);
    drawMesh(SUPERSHAPE_COUNT,0,0,0,1,0,1,0);
    commands->lpVtbl->SetPipelineState(commands,modelPipeline);
    drawModels(tick,1);
    barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET; barrier.Transition.StateAfter=after;
    commands->lpVtbl->ResourceBarrier(commands,1,&barrier);
}

static void submit(void)
{
    ID3D12CommandList *list=(ID3D12CommandList *)commands;
    check(commands->lpVtbl->Close(commands));
    queue->lpVtbl->ExecuteCommandLists(queue,1,&list);
}

static void waitGPU(void)
{
    /* One frame in flight minimizes code and makes allocator reuse safe. */
    ++fenceValue;
    check(queue->lpVtbl->Signal(queue,fence,fenceValue));
    /* A null event makes this API wait synchronously for the fence value. */
    check(fence->lpVtbl->SetEventOnCompletion(fence,fenceValue,0));
}

#ifdef _DEBUG
static void releaseRenderer(void)
{
    commands->lpVtbl->Release(commands); allocator->lpVtbl->Release(allocator);
    modelPipeline->lpVtbl->Release(modelPipeline); floorPipeline->lpVtbl->Release(floorPipeline);
    rootSignature->lpVtbl->Release(rootSignature);
    vertices->lpVtbl->Release(vertices); depth->lpVtbl->Release(depth);
    rtvHeap->lpVtbl->Release(rtvHeap); dsvHeap->lpVtbl->Release(dsvHeap);
    fence->lpVtbl->Release(fence); queue->lpVtbl->Release(queue);
}
#endif

__declspec(naked) void __cdecl winmain(void)
{
    __asm {
        push ebp
        mov ebp,esp
        sub esp,__LOCAL_SIZE
    }
    {
        static DXGI_SWAP_CHAIN_DESC desc={
            {WINWIDTH,WINHEIGHT,{0,0},DXGI_FORMAT_R8G8B8A8_UNORM,0,0},
            {1,0},DXGI_USAGE_RENDER_TARGET_OUTPUT,FRAMECOUNT,0,TRUE,DXGI_SWAP_EFFECT_FLIP_DISCARD,0
        };
        IDXGIFactory4 *factory;
        IDXGISwapChain *baseSwapChain;
        IDXGISwapChain3 *swapChain;
        ID3D12Resource *backBuffers[FRAMECOUNT];
        D3D12_CPU_DESCRIPTOR_HANDLE view;
        UINT i,increment;
        DWORD start,tick;
        MSG message;
        HWND window=CreateWindowW(L"edit",WINDOW_TITLE,WS_POPUP|WS_VISIBLE,100,100,WINWIDTH,WINHEIGHT,0,0,0,0);
        if (!window) ExitProcess(1);
#ifdef _DEBUG
        {
            ID3D12Debug *debug;
            if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug,(void **)&debug))) {
                debug->lpVtbl->EnableDebugLayer(debug); debug->lpVtbl->Release(debug);
            }
        }
#endif
        check(CreateDXGIFactory1(&IID_IDXGIFactory4,(void **)&factory));
        check(D3D12CreateDevice(0,D3D_FEATURE_LEVEL_11_0,&IID_ID3D12Device,(void **)&device));
        initializeRenderer();
        desc.OutputWindow=window;
        check(factory->lpVtbl->CreateSwapChain(factory,(IUnknown *)queue,&desc,&baseSwapChain));
        check(baseSwapChain->lpVtbl->QueryInterface(baseSwapChain,&IID_IDXGISwapChain3,(void **)&swapChain));
        baseSwapChain->lpVtbl->Release(baseSwapChain); factory->lpVtbl->Release(factory);
        increment=device->lpVtbl->GetDescriptorHandleIncrementSize(device,D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        view=heapStart(rtvHeap);
        for (i=0;i<FRAMECOUNT;++i) {
            check(swapChain->lpVtbl->GetBuffer(swapChain,i,&IID_ID3D12Resource,(void **)&backBuffers[i]));
            device->lpVtbl->CreateRenderTargetView(device,backBuffers[i],0,view);
            view.ptr+=increment;
        }
        start=GetTickCount();
        for (;;) {
            while (PeekMessageW(&message,0,0,0,PM_REMOVE)) {
                if (message.message==WM_QUIT || message.message==WM_CLOSE ||
                    (message.message==WM_KEYDOWN && message.wParam==VK_ESCAPE)) ExitProcess(0);
                DispatchMessageW(&message);
            }
            tick=GetTickCount()-start;
            if (tick>=SA_RUN_LENGTH) break;
            i=swapChain->lpVtbl->GetCurrentBackBufferIndex(swapChain);
            view=heapStart(rtvHeap); view.ptr+=i*increment;
            recordFrame(tick,backBuffers[i],view,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_PRESENT);
            submit();
            check(swapChain->lpVtbl->Present(swapChain,1,0));
            waitGPU();
        }
#ifdef _DEBUG
        for (i=0;i<FRAMECOUNT;++i) backBuffers[i]->lpVtbl->Release(backBuffers[i]);
        swapChain->lpVtbl->Release(swapChain);
        releaseRenderer(); device->lpVtbl->Release(device);
        DestroyWindow(window);
#endif
    }
    ExitProcess(0);
}
