/* San Angeles Observation - DirectX 11, Win32, no C runtime.
 * Adapted from Jetro Lauha's 2004-2005 OpenGL ES demonstration.
 * Copyright (c) 2004-2005 Jetro Lauha. See ../../San Angeles/license-BSD.txt.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include "../../San Angeles/Scene.h"
#include "vertex.sh"
#include "pixel.sh"

#define WINWIDTH 1280
#define WINHEIGHT 720
#ifdef _DEBUG
#define WINDOW_TITLE L"San Angeles - DirectX 11"
#else
#define WINDOW_TITLE 0
#endif
static const GUID textureIID = { 0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c} };
static ID3D11Device *device;
static ID3D11DeviceContext *context;
static IDXGISwapChain *swapChain;
static ID3D11Buffer *constantBuffer;
static SA_CONSTANTS constants;

static void check(HRESULT result) { if (FAILED(result)) ExitProcess((UINT)result); }

static void drawMesh(unsigned int mesh, float x, float y, float z, float scale, float angle, float mirror, float lit)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    constants.translateScale[0]=x; constants.translateScale[1]=y;
    constants.translateScale[2]=z; constants.translateScale[3]=scale;
    constants.rotateMirrorLight[0]=saCos(angle); constants.rotateMirrorLight[1]=saSin(angle);
    constants.rotateMirrorLight[2]=mirror; constants.rotateMirrorLight[3]=lit;
    check(context->lpVtbl->Map(context,(ID3D11Resource *)constantBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mapped));
    memcpy(mapped.pData,&constants,sizeof(constants));
    context->lpVtbl->Unmap(context,(ID3D11Resource *)constantBuffer,0);
    context->lpVtbl->Draw(context,saMeshes[mesh].count,saMeshes[mesh].first);
}

static void drawModels(unsigned int tick, float mirror)
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

__declspec(naked) void __cdecl winmain(void)
{
    __asm {
        push ebp
        mov ebp,esp
        sub esp,__LOCAL_SIZE
    }
    {
        static const DXGI_SWAP_CHAIN_DESC swapDesc = {
            {WINWIDTH,WINHEIGHT,{60,1},DXGI_FORMAT_R8G8B8A8_UNORM,0,0},
            {1,0},DXGI_USAGE_RENDER_TARGET_OUTPUT,1,0,TRUE,DXGI_SWAP_EFFECT_DISCARD,0
        };
        static const D3D11_INPUT_ELEMENT_DESC elements[] = {
            {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}
        };
        static const D3D11_VIEWPORT viewport = {0,0,WINWIDTH,WINHEIGHT,0,1};
        static const D3D11_RASTERIZER_DESC rasterDesc = {D3D11_FILL_SOLID,D3D11_CULL_NONE,FALSE,0,0,0,TRUE,FALSE,FALSE,FALSE};
        static const D3D11_BLEND_DESC blendDesc = {FALSE,FALSE,{{TRUE,D3D11_BLEND_ZERO,D3D11_BLEND_SRC_COLOR,D3D11_BLEND_OP_ADD,D3D11_BLEND_ZERO,D3D11_BLEND_ONE,D3D11_BLEND_OP_ADD,D3D11_COLOR_WRITE_ENABLE_ALL}}};
        static const D3D11_DEPTH_STENCIL_DESC noDepthDesc = {FALSE,D3D11_DEPTH_WRITE_MASK_ZERO,D3D11_COMPARISON_ALWAYS,FALSE,0,0,{D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS},{D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS}};
        static D3D11_TEXTURE2D_DESC depthDesc = {WINWIDTH,WINHEIGHT,1,1,DXGI_FORMAT_D24_UNORM_S8_UINT,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_DEPTH_STENCIL,0,0};
        DXGI_SWAP_CHAIN_DESC sd=swapDesc;
        D3D11_BUFFER_DESC bufferDesc={0};
        D3D11_SUBRESOURCE_DATA data={0};
        ID3D11Texture2D *backBuffer,*depth;
        ID3D11RenderTargetView *target;
        ID3D11DepthStencilView *depthView;
        ID3D11DepthStencilState *noDepth;
        ID3D11BlendState *floorBlend;
        ID3D11RasterizerState *raster;
        ID3D11VertexShader *vertexShader;
        ID3D11PixelShader *pixelShader;
        ID3D11InputLayout *layout;
        ID3D11Buffer *vertices;
        UINT stride=sizeof(SA_VERTEX),offset=0,flags=D3D11_CREATE_DEVICE_SINGLETHREADED;
        DWORD start,tick;
        MSG message;
        HWND window=CreateWindowW(L"edit",WINDOW_TITLE,WS_POPUP|WS_VISIBLE,100,100,WINWIDTH,WINHEIGHT,0,0,0,0);
        if (!window) ExitProcess(1);
        sd.OutputWindow=window;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        check(D3D11CreateDeviceAndSwapChain(0,D3D_DRIVER_TYPE_HARDWARE,0,flags,0,0,D3D11_SDK_VERSION,&sd,&swapChain,&device,0,&context));
        check(swapChain->lpVtbl->GetBuffer(swapChain,0,&textureIID,(void **)&backBuffer));
        check(device->lpVtbl->CreateRenderTargetView(device,(ID3D11Resource *)backBuffer,0,&target));
        backBuffer->lpVtbl->Release(backBuffer);
        check(device->lpVtbl->CreateTexture2D(device,&depthDesc,0,&depth));
        check(device->lpVtbl->CreateDepthStencilView(device,(ID3D11Resource *)depth,0,&depthView));
        depth->lpVtbl->Release(depth);
        check(device->lpVtbl->CreateRasterizerState(device,&rasterDesc,&raster));
        check(device->lpVtbl->CreateBlendState(device,&blendDesc,&floorBlend));
        check(device->lpVtbl->CreateDepthStencilState(device,&noDepthDesc,&noDepth));
        check(device->lpVtbl->CreateVertexShader(device,g_VShader,sizeof(g_VShader),0,&vertexShader));
        check(device->lpVtbl->CreatePixelShader(device,g_PShader,sizeof(g_PShader),0,&pixelShader));
        check(device->lpVtbl->CreateInputLayout(device,elements,3,g_VShader,sizeof(g_VShader),&layout));
        saCreateScene();
        bufferDesc.ByteWidth=saVertexCount*sizeof(SA_VERTEX);
        bufferDesc.Usage=D3D11_USAGE_IMMUTABLE; bufferDesc.BindFlags=D3D11_BIND_VERTEX_BUFFER;
        data.pSysMem=saVertices;
        check(device->lpVtbl->CreateBuffer(device,&bufferDesc,&data,&vertices));
        bufferDesc.ByteWidth=sizeof(constants); bufferDesc.Usage=D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags=D3D11_BIND_CONSTANT_BUFFER; bufferDesc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        check(device->lpVtbl->CreateBuffer(device,&bufferDesc,0,&constantBuffer));
        context->lpVtbl->OMSetRenderTargets(context,1,&target,depthView);
        context->lpVtbl->RSSetState(context,raster);
        context->lpVtbl->RSSetViewports(context,1,&viewport);
        context->lpVtbl->IASetInputLayout(context,layout);
        context->lpVtbl->IASetPrimitiveTopology(context,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->lpVtbl->IASetVertexBuffers(context,0,1,&vertices,&stride,&offset);
        context->lpVtbl->VSSetShader(context,vertexShader,0,0);
        context->lpVtbl->PSSetShader(context,pixelShader,0,0);
        context->lpVtbl->VSSetConstantBuffers(context,0,1,&constantBuffer);
        constants.right[3]=2.41421356237f*WINHEIGHT/WINWIDTH;
        start=GetTickCount();
        for (;;) {
            float clear[4];
            while (PeekMessageW(&message,0,0,0,PM_REMOVE)) {
                if (message.message==WM_QUIT || message.message==WM_CLOSE ||
                    (message.message==WM_KEYDOWN && message.wParam==VK_ESCAPE)) ExitProcess(0);
                TranslateMessage(&message); DispatchMessageW(&message);
            }
            tick=GetTickCount()-start;
            if (tick>=SA_RUN_LENGTH) break;
            saCamera(tick,&constants);
            clear[0]=0.1f*constants.eye[3]; clear[1]=0.2f*constants.eye[3]; clear[2]=0.3f*constants.eye[3]; clear[3]=1;
            context->lpVtbl->ClearRenderTargetView(context,target,clear);
            context->lpVtbl->ClearDepthStencilView(context,depthView,D3D11_CLEAR_DEPTH,1,0);
            drawModels(tick,-1);
            context->lpVtbl->OMSetDepthStencilState(context,noDepth,0);
            context->lpVtbl->OMSetBlendState(context,floorBlend,0,0xffffffffu);
            drawMesh(SUPERSHAPE_COUNT,0,0,0,1,0,1,0);
            context->lpVtbl->OMSetBlendState(context,0,0,0xffffffffu);
            context->lpVtbl->OMSetDepthStencilState(context,0,0);
            drawModels(tick,1);
            check(swapChain->lpVtbl->Present(swapChain,1,0));
        }
#ifdef _DEBUG
        /* The packed intro exits the process immediately; Windows reclaims
         * these resources. Keep explicit teardown in the debugging build. */
        context->lpVtbl->ClearState(context);
        vertices->lpVtbl->Release(vertices); constantBuffer->lpVtbl->Release(constantBuffer);
        layout->lpVtbl->Release(layout); vertexShader->lpVtbl->Release(vertexShader); pixelShader->lpVtbl->Release(pixelShader);
        raster->lpVtbl->Release(raster); floorBlend->lpVtbl->Release(floorBlend); noDepth->lpVtbl->Release(noDepth);
        target->lpVtbl->Release(target); depthView->lpVtbl->Release(depthView);
        swapChain->lpVtbl->Release(swapChain); context->lpVtbl->Release(context); device->lpVtbl->Release(device);
        DestroyWindow(window);
#endif
    }
    ExitProcess(0);
}
