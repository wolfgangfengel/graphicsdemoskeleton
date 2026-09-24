// Himalayas by Reinder Nijhoff (2018). See ../../Himalayas/LICENSE.txt.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_4.h>
struct ID3D10Effect;
#include <d3dcompiler.h>
#include "../../Himalayas/Input.h"
#include "../../Himalayas/Sources.h"
static ID3D12Device *hDevice;
static IDXGISwapChain3 *hSwapchain;
static ID3D12Resource *hTextures[7],*hBackbuffers[2];
static ID3D12CommandQueue *hQueue;
static ID3D12CommandAllocator *hAllocator;
static ID3D12GraphicsCommandList *hCommands;
static ID3D12DescriptorHeap *hHeap,*hClearHeap;
static ID3D12RootSignature *hRoot;
static ID3D12PipelineState *hShaders[5];
static ID3D12Fence *hFence;
static UINT hFenceValue,hDescriptorSize;
static D3D12_CPU_DESCRIPTOR_HANDLE hCPU,hClearCPU;
static D3D12_GPU_DESCRIPTOR_HANDLE hGPU;
static void hCheck(HRESULT result) {if(FAILED(result)) ExitProcess((UINT)result);}
static D3D12_CPU_DESCRIPTOR_HANDLE hCpu(UINT index) {D3D12_CPU_DESCRIPTOR_HANDLE result=hCPU;result.ptr+=index*hDescriptorSize;return result;}
static D3D12_GPU_DESCRIPTOR_HANDLE hGpu(UINT index) {D3D12_GPU_DESCRIPTOR_HANDLE result=hGPU;result.ptr+=index*hDescriptorSize;return result;}
static void hTransition(ID3D12Resource *resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier={0};
    barrier.Transition.pResource=resource;barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore=before;barrier.Transition.StateAfter=after;
    hCommands->lpVtbl->ResourceBarrier(hCommands,1,&barrier);
}
static void hInitialize(HWND window)
{
    static DXGI_SWAP_CHAIN_DESC swap={{WINWIDTH,WINHEIGHT,{0,0},DXGI_FORMAT_R8G8B8A8_UNORM,0,0},{1,0},DXGI_USAGE_RENDER_TARGET_OUTPUT,2,0,TRUE,DXGI_SWAP_EFFECT_FLIP_DISCARD,0};
    static const D3D12_COMMAND_QUEUE_DESC queue={0};
    static const D3D12_HEAP_PROPERTIES memory={D3D12_HEAP_TYPE_DEFAULT,0,0,1,1};
    static D3D12_RESOURCE_DESC texture={D3D12_RESOURCE_DIMENSION_TEXTURE2D,0,WINWIDTH,WINHEIGHT,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,{1,0},D3D12_TEXTURE_LAYOUT_UNKNOWN,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS};
    static const D3D12_DESCRIPTOR_HEAP_DESC heap={D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,57,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    static const D3D12_DESCRIPTOR_HEAP_DESC clearHeap={D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,7,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
    static D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0}};
    static D3D12_ROOT_PARAMETER parameters[3];
    static D3D12_STATIC_SAMPLER_DESC samplers[2];
    static D3D12_ROOT_SIGNATURE_DESC root={3,parameters,2,samplers,0};
    static D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline;
    static D3D12_SHADER_RESOURCE_VIEW_DESC view;
    IDXGIFactory4 *factory;
    IDXGISwapChain *swapchain;
    ID3DBlob *code;
    UINT n,p,parity,output;
    int inputs[4];
#ifdef _DEBUG
    {
        ID3D12Debug *debug;
        hCheck(D3D12GetDebugInterface(&IID_ID3D12Debug,(void **)&debug));
        debug->lpVtbl->EnableDebugLayer(debug);debug->lpVtbl->Release(debug);
    }
#endif
    hCheck(CreateDXGIFactory1(&IID_IDXGIFactory4,(void **)&factory));
    hCheck(D3D12CreateDevice(0,D3D_FEATURE_LEVEL_11_0,&IID_ID3D12Device,(void **)&hDevice));
    hCheck(hDevice->lpVtbl->CreateCommandQueue(hDevice,&queue,&IID_ID3D12CommandQueue,(void **)&hQueue));
    swap.OutputWindow=window;
    hCheck(factory->lpVtbl->CreateSwapChain(factory,(IUnknown *)hQueue,&swap,&swapchain));
    hCheck(swapchain->lpVtbl->QueryInterface(swapchain,&IID_IDXGISwapChain3,(void **)&hSwapchain));
    swapchain->lpVtbl->Release(swapchain);factory->lpVtbl->Release(factory);
    for(n=0;n<2;++n) hCheck(hSwapchain->lpVtbl->GetBuffer(hSwapchain,n,&IID_ID3D12Resource,(void **)&hBackbuffers[n]));
    hCheck(hDevice->lpVtbl->CreateDescriptorHeap(hDevice,&heap,&IID_ID3D12DescriptorHeap,(void **)&hHeap));
    hCheck(hDevice->lpVtbl->CreateDescriptorHeap(hDevice,&clearHeap,&IID_ID3D12DescriptorHeap,(void **)&hClearHeap));
    /* Explicit hidden result pointer: x86 C++ COM struct-return ABI. */
    ((void (__stdcall *)(ID3D12DescriptorHeap *,D3D12_CPU_DESCRIPTOR_HANDLE *))hHeap->lpVtbl->GetCPUDescriptorHandleForHeapStart)(hHeap,&hCPU);
    ((void (__stdcall *)(ID3D12DescriptorHeap *,D3D12_GPU_DESCRIPTOR_HANDLE *))hHeap->lpVtbl->GetGPUDescriptorHandleForHeapStart)(hHeap,&hGPU);
    ((void (__stdcall *)(ID3D12DescriptorHeap *,D3D12_CPU_DESCRIPTOR_HANDLE *))hClearHeap->lpVtbl->GetCPUDescriptorHandleForHeapStart)(hClearHeap,&hClearCPU);
    hDescriptorSize=hDevice->lpVtbl->GetDescriptorHandleIncrementSize(hDevice,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for(n=0;n<7;++n) {
        D3D12_CPU_DESCRIPTOR_HANDLE clear=hClearCPU;
        clear.ptr+=n*hDescriptorSize;
        if(n==6) texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        hCheck(hDevice->lpVtbl->CreateCommittedResource(hDevice,&memory,0,&texture,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,0,&IID_ID3D12Resource,(void **)&hTextures[n]));
        hDevice->lpVtbl->CreateUnorderedAccessView(hDevice,hTextures[n],0,0,hCpu(n));
        hDevice->lpVtbl->CreateUnorderedAccessView(hDevice,hTextures[n],0,0,clear);
    }
    view.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;
    for(parity=0;parity<2;++parity) for(p=0;p<5;++p) {
        UINT base=7+(parity*5+p)*5;
        hPassImages(p,parity,&output,inputs);
        hDevice->lpVtbl->CreateUnorderedAccessView(hDevice,hTextures[output],0,0,hCpu(base));
        for(n=0;n<4;++n) hDevice->lpVtbl->CreateShaderResourceView(hDevice,inputs[n]<0?0:hTextures[inputs[n]],&view,hCpu(base+1+n));
    }
    parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[0].Constants.Num32BitValues=sizeof(hConstants)/sizeof(float);
    for(n=0;n<2;++n) {
        parameters[n+1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[n+1].DescriptorTable.NumDescriptorRanges=1;parameters[n+1].DescriptorTable.pDescriptorRanges=&ranges[n];
        samplers[n].Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplers[n].AddressU=samplers[n].AddressV=samplers[n].AddressW=n?D3D12_TEXTURE_ADDRESS_MODE_WRAP:D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[n].MaxAnisotropy=1;samplers[n].ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
        samplers[n].MaxLOD=D3D12_FLOAT32_MAX;samplers[n].ShaderRegister=n;
    }
    hCheck(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&code,0));
    hCheck(hDevice->lpVtbl->CreateRootSignature(hDevice,0,code->lpVtbl->GetBufferPointer(code),code->lpVtbl->GetBufferSize(code),&IID_ID3D12RootSignature,(void **)&hRoot));
    code->lpVtbl->Release(code);pipeline.pRootSignature=hRoot;
    for(n=0;n<5;++n) {
        hCheck(D3DCompile(source_CS_A,sizeof(source_CS_A)-1,0,0,0,hEntryNames[n],"cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&code,0));
        pipeline.CS.pShaderBytecode=code->lpVtbl->GetBufferPointer(code);pipeline.CS.BytecodeLength=code->lpVtbl->GetBufferSize(code);
        hCheck(hDevice->lpVtbl->CreateComputePipelineState(hDevice,&pipeline,&IID_ID3D12PipelineState,(void **)&hShaders[n]));
        code->lpVtbl->Release(code);
    }
    hCheck(hDevice->lpVtbl->CreateCommandAllocator(hDevice,D3D12_COMMAND_LIST_TYPE_DIRECT,&IID_ID3D12CommandAllocator,(void **)&hAllocator));
    hCheck(hDevice->lpVtbl->CreateCommandList(hDevice,0,D3D12_COMMAND_LIST_TYPE_DIRECT,hAllocator,0,&IID_ID3D12GraphicsCommandList,(void **)&hCommands));
    hCheck(hCommands->lpVtbl->Close(hCommands));
    hCheck(hDevice->lpVtbl->CreateFence(hDevice,0,0,&IID_ID3D12Fence,(void **)&hFence));
}
static void hRender(void)
{
    static const float zero[4]={0};
    UINT pass,n,output;
    int inputs[4];
    ID3D12Resource *target=hBackbuffers[hSwapchain->lpVtbl->GetCurrentBackBufferIndex(hSwapchain)];
    hCheck(hAllocator->lpVtbl->Reset(hAllocator));hCheck(hCommands->lpVtbl->Reset(hCommands,hAllocator,0));
    hCommands->lpVtbl->SetDescriptorHeaps(hCommands,1,&hHeap);
    hCommands->lpVtbl->SetComputeRootSignature(hCommands,hRoot);
    hCommands->lpVtbl->SetComputeRoot32BitConstants(hCommands,0,sizeof(hConstants)/sizeof(float),hConstants,0);
    if(!hFrameNumber) for(n=0;n<6;++n) {
        D3D12_CPU_DESCRIPTOR_HANDLE clear=hClearCPU;
        clear.ptr+=n*hDescriptorSize;
        /* Clear reads its CPU descriptor: it must be in a nonvisible heap. */
        hCommands->lpVtbl->ClearUnorderedAccessViewFloat(hCommands,hGpu(n),clear,hTextures[n],zero,0,0);
        hTransition(hTextures[n],D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    for(pass=hFrameNumber?2:0;pass<5;++pass) {
        UINT base=7+((hFrameNumber&1)*5+pass)*5;
        hPassImages(pass,hFrameNumber&1,&output,inputs);
        if(output!=6) hTransition(hTextures[output],D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        hCommands->lpVtbl->SetPipelineState(hCommands,hShaders[pass]);
        hCommands->lpVtbl->SetComputeRootDescriptorTable(hCommands,1,hGpu(base));
        hCommands->lpVtbl->SetComputeRootDescriptorTable(hCommands,2,hGpu(base+1));
        hCommands->lpVtbl->Dispatch(hCommands,(WINWIDTH+7)/8,(WINHEIGHT+7)/8,1);
        if(output!=6) hTransition(hTextures[output],D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    hTransition(hTextures[6],D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    hTransition(target,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_DEST);
    hCommands->lpVtbl->CopyResource(hCommands,target,hTextures[6]);
    hTransition(target,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PRESENT);
    hTransition(hTextures[6],D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    hCheck(hCommands->lpVtbl->Close(hCommands));
    hQueue->lpVtbl->ExecuteCommandLists(hQueue,1,(ID3D12CommandList *const *)&hCommands);
}
static void hPresent(void)
{
    hCheck(hSwapchain->lpVtbl->Present(hSwapchain,0,0));
    hCheck(hQueue->lpVtbl->Signal(hQueue,hFence,++hFenceValue));
    hCheck(hFence->lpVtbl->SetEventOnCompletion(hFence,hFenceValue,0));
}
static void hShutdown(void)
{
    UINT n;
    for(n=0;n<7;++n) hTextures[n]->lpVtbl->Release(hTextures[n]);
    for(n=0;n<2;++n) hBackbuffers[n]->lpVtbl->Release(hBackbuffers[n]);
    for(n=0;n<5;++n) hShaders[n]->lpVtbl->Release(hShaders[n]);
    hRoot->lpVtbl->Release(hRoot);hHeap->lpVtbl->Release(hHeap);hClearHeap->lpVtbl->Release(hClearHeap);hCommands->lpVtbl->Release(hCommands);
    hAllocator->lpVtbl->Release(hAllocator);hFence->lpVtbl->Release(hFence);hSwapchain->lpVtbl->Release(hSwapchain);
    hQueue->lpVtbl->Release(hQueue);hDevice->lpVtbl->Release(hDevice);
}
#include "../../Himalayas/Entry.h"
