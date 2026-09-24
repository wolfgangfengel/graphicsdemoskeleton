////////////////////////////////////////////////////////////////////////
// Mountains by David Hoskins, 2013. HLSL port: see LICENSE-Mountains.txt.
// DirectX 11 compute shader, custom x86 entry point, no C runtime.
////////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "TerrainSource.h"

#define WINWIDTH 1280
#define WINHEIGHT 720

/* One constant-buffer register: seconds, resolution, and mouse time shift. */
static float mConstants[4] = { 0, WINWIDTH, WINHEIGHT, 0 };

static void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result)) ExitProcess((UINT)result);
}

__declspec(naked) void __cdecl winmain(void)
{
    __asm {
        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE
    }
    {
        static DXGI_SWAP_CHAIN_DESC swapDesc = {
            { WINWIDTH, WINHEIGHT, { 60, 1 }, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0 },
            { 1, 0 }, DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS,
            1, NULL, TRUE, DXGI_SWAP_EFFECT_SEQUENTIAL, 0
        };
        static const D3D11_BUFFER_DESC bufferDesc = {
            sizeof(mConstants), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0
        };
        ID3D11Device *device;
        ID3D11DeviceContext *context;
        IDXGISwapChain *swapChain;
        ID3D11Texture2D *texture;
        ID3D11Buffer *constants;
        ID3D11UnorderedAccessView *output;
        ID3D11ComputeShader *shader;
        ID3DBlob *bytecode;
        DWORD start, elapsed;
        MSG message;
        HWND window = CreateWindowW(L"edit", NULL, WS_POPUP | WS_VISIBLE,
            200, 200, WINWIDTH, WINHEIGHT, NULL, NULL, NULL, NULL);
        if (!window) ExitProcess(1);
        ShowCursor(FALSE);
        swapDesc.OutputWindow = window;
        ThrowIfFailed(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
#ifdef _DEBUG
            D3D11_CREATE_DEVICE_DEBUG,
#else
            D3D11_CREATE_DEVICE_SINGLETHREADED,
#endif
            NULL, 0, D3D11_SDK_VERSION, &swapDesc, &swapChain, &device, NULL, &context));
        ThrowIfFailed(swapChain->lpVtbl->GetBuffer(swapChain, 0, &IID_ID3D11Texture2D, (void **)&texture));
        ThrowIfFailed(device->lpVtbl->CreateBuffer(device, &bufferDesc, NULL, &constants));
        ThrowIfFailed(device->lpVtbl->CreateUnorderedAccessView(device, (ID3D11Resource *)texture, NULL, &output));
        ThrowIfFailed(D3DCompile(terrainSource, sizeof(terrainSource)-1, NULL, NULL, NULL,
            "CS_Terrain", "cs_5_0", 0, 0, &bytecode, NULL));
        ThrowIfFailed(device->lpVtbl->CreateComputeShader(device,
            bytecode->lpVtbl->GetBufferPointer(bytecode), bytecode->lpVtbl->GetBufferSize(bytecode), NULL, &shader));
        bytecode->lpVtbl->Release(bytecode);
        context->lpVtbl->CSSetShader(context, shader, NULL, 0);
        context->lpVtbl->CSSetConstantBuffers(context, 0, 1, &constants);

        start = GetTickCount();
        for (;;) {
            while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT || message.message == WM_CLOSE ||
                    (message.hwnd == window && message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE)) goto finished;
                if (message.hwnd == window && (message.message == WM_LBUTTONDOWN ||
                    (message.message == WM_MOUSEMOVE && (message.wParam & MK_LBUTTON)))) {
                    int x = (short)LOWORD(message.lParam);
                    mConstants[3] = (float)(x < 0 ? 0 : x > WINWIDTH ? WINWIDTH : x);
                }
                DispatchMessageW(&message);
            }
            elapsed = GetTickCount() - start;
            if (elapsed > 30000) break;
            mConstants[0] = (int)elapsed * 0.001f;
            context->lpVtbl->UpdateSubresource(context, (ID3D11Resource *)constants, 0, NULL, mConstants, 0, 0);
            context->lpVtbl->CSSetUnorderedAccessViews(context, 0, 1, &output, NULL);
            context->lpVtbl->Dispatch(context, (WINWIDTH + 7) / 8, (WINHEIGHT + 7) / 8, 1);
            ThrowIfFailed(swapChain->lpVtbl->Present(swapChain, 1, 0));
        }
finished:
#ifdef _DEBUG
        context->lpVtbl->ClearState(context);
        constants->lpVtbl->Release(constants);
        shader->lpVtbl->Release(shader);
        output->lpVtbl->Release(output);
        texture->lpVtbl->Release(texture);
        swapChain->lpVtbl->Release(swapChain);
        context->lpVtbl->Release(context);
        device->lpVtbl->Release(device);
        ShowCursor(TRUE);
        DestroyWindow(window);
#else
        ; /* Release relies on process teardown, as in the other tiny demos. */
#endif
    }
    ExitProcess(0);
}
