// Himalayas by Reinder Nijhoff (2018). See ../../Himalayas/LICENSE.txt.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "../../Himalayas/Input.h"
#include "../../Himalayas/Sources.h"
static ID3D11Device *hDevice;
static ID3D11DeviceContext *hContext;
static IDXGISwapChain *hSwapchain;
static ID3D11Texture2D *hTextures[7],*hBackbuffer;
static ID3D11ShaderResourceView *hSRV[6];
static ID3D11UnorderedAccessView *hUAV[7];
static ID3D11ComputeShader *hShaders[5];
static ID3D11Buffer *hUniform;
static ID3D11SamplerState *hSamplers[2];
static void hCheck(HRESULT result) {if(FAILED(result)) ExitProcess((UINT)result);}
static void hInitialize(HWND window)
{
    static DXGI_SWAP_CHAIN_DESC swap={ {WINWIDTH,WINHEIGHT,{60,1},DXGI_FORMAT_R8G8B8A8_UNORM,0,0},{1,0},DXGI_USAGE_RENDER_TARGET_OUTPUT,1,0,TRUE,DXGI_SWAP_EFFECT_SEQUENTIAL,0};
    static D3D11_TEXTURE2D_DESC texture={WINWIDTH,WINHEIGHT,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS,0,0};
    static D3D11_BUFFER_DESC uniform={sizeof(hConstants),D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};
    static D3D11_SAMPLER_DESC sampler={D3D11_FILTER_MIN_MAG_MIP_LINEAR,D3D11_TEXTURE_ADDRESS_CLAMP,D3D11_TEXTURE_ADDRESS_CLAMP,D3D11_TEXTURE_ADDRESS_CLAMP,0,1,D3D11_COMPARISON_ALWAYS,{0},0,D3D11_FLOAT32_MAX};
    static const float zero[4]={0};
    UINT n;
    swap.OutputWindow=window;
    hCheck(D3D11CreateDeviceAndSwapChain(0,D3D_DRIVER_TYPE_HARDWARE,0,
#ifdef _DEBUG
        D3D11_CREATE_DEVICE_DEBUG,
#else
        D3D11_CREATE_DEVICE_SINGLETHREADED,
#endif
        0,0,D3D11_SDK_VERSION,&swap,&hSwapchain,&hDevice,0,&hContext));
    hCheck(hSwapchain->lpVtbl->GetBuffer(hSwapchain,0,&IID_ID3D11Texture2D,(void **)&hBackbuffer));
    for(n=0;n<7;++n) {
        if(n==6) {texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM;texture.BindFlags=D3D11_BIND_UNORDERED_ACCESS;}
        hCheck(hDevice->lpVtbl->CreateTexture2D(hDevice,&texture,0,&hTextures[n]));
        hCheck(hDevice->lpVtbl->CreateUnorderedAccessView(hDevice,(ID3D11Resource *)hTextures[n],0,&hUAV[n]));
        if(n<6) hCheck(hDevice->lpVtbl->CreateShaderResourceView(hDevice,(ID3D11Resource *)hTextures[n],0,&hSRV[n]));
        hContext->lpVtbl->ClearUnorderedAccessViewFloat(hContext,hUAV[n],zero);
    }
    hCheck(hDevice->lpVtbl->CreateBuffer(hDevice,&uniform,0,&hUniform));
    for(n=0;n<2;++n) {
        hCheck(hDevice->lpVtbl->CreateSamplerState(hDevice,&sampler,&hSamplers[n]));
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
    }
    for(n=0;n<5;++n) {
        ID3DBlob *code;
        hCheck(D3DCompile(source_CS_A,sizeof(source_CS_A)-1,0,0,0,hEntryNames[n],"cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&code,0));
        hCheck(hDevice->lpVtbl->CreateComputeShader(hDevice,code->lpVtbl->GetBufferPointer(code),code->lpVtbl->GetBufferSize(code),0,&hShaders[n]));
        code->lpVtbl->Release(code);
    }
    hContext->lpVtbl->CSSetConstantBuffers(hContext,0,1,&hUniform);
    hContext->lpVtbl->CSSetSamplers(hContext,0,2,hSamplers);
}
static void hRender(void)
{
    UINT pass,n,output;
    int inputs[4];
    ID3D11ShaderResourceView *views[4];
    static ID3D11UnorderedAccessView *none;
    static ID3D11ShaderResourceView *noViews[4];
    hContext->lpVtbl->UpdateSubresource(hContext,(ID3D11Resource *)hUniform,0,0,hConstants,0,0);
    for(pass=hFrameNumber?2:0;pass<5;++pass) {
        hPassImages(pass,hFrameNumber&1,&output,inputs);
        for(n=0;n<4;++n) views[n]=inputs[n]<0?0:hSRV[inputs[n]];
        hContext->lpVtbl->CSSetShaderResources(hContext,0,4,noViews);
        hContext->lpVtbl->CSSetUnorderedAccessViews(hContext,0,1,&hUAV[output],0);
        hContext->lpVtbl->CSSetShaderResources(hContext,0,4,views);
        hContext->lpVtbl->CSSetShader(hContext,hShaders[pass],0,0);
        hContext->lpVtbl->Dispatch(hContext,(WINWIDTH+7)/8,(WINHEIGHT+7)/8,1);
        hContext->lpVtbl->CSSetUnorderedAccessViews(hContext,0,1,&none,0);
    }
    hContext->lpVtbl->CopyResource(hContext,(ID3D11Resource *)hBackbuffer,(ID3D11Resource *)hTextures[6]);
}
static void hPresent(void) {hCheck(hSwapchain->lpVtbl->Present(hSwapchain,0,0));}
static void hShutdown(void)
{
    UINT n;
    hContext->lpVtbl->ClearState(hContext);
    for(n=0;n<7;++n) {hUAV[n]->lpVtbl->Release(hUAV[n]);hTextures[n]->lpVtbl->Release(hTextures[n]);if(n<6)hSRV[n]->lpVtbl->Release(hSRV[n]);}
    for(n=0;n<5;++n) hShaders[n]->lpVtbl->Release(hShaders[n]);
    for(n=0;n<2;++n) hSamplers[n]->lpVtbl->Release(hSamplers[n]);
    hUniform->lpVtbl->Release(hUniform);hBackbuffer->lpVtbl->Release(hBackbuffer);
    hSwapchain->lpVtbl->Release(hSwapchain);hContext->lpVtbl->Release(hContext);hDevice->lpVtbl->Release(hDevice);
}
#include "../../Himalayas/Entry.h"
