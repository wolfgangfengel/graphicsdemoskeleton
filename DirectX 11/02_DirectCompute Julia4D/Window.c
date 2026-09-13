////////////////////////////////////////////////////////////////////////
// Port of Jan Vlietnick's Julia 4D demo by Wolfgang Engel.
// DirectX 11 compute shader, custom x86 entry point, no C runtime.
////////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <d3d11.h>
#include "qjulia4D.sh"

#define WINWIDTH 1280
#define WINHEIGHT 720
#define WINPOSX 200
#define WINPOSY 200

/* Matches cbCS, including the final constant-buffer register's padding. */
typedef struct MainConstantBuffer {
    float diffuse[4], mu[4], epsilon;
    int width, height, selfShadow;
    float orientation[16], zoom, padding[3];
} MainConstantBuffer;

static MainConstantBuffer mConstants = {
    { 0 }, { 0 }, 0.003f, WINWIDTH, WINHEIGHT, TRUE,
    { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }, 1, { 0 }
};
static float mMuA[4] = { -0.278f, -0.479f, 0, 0 };
static float mMuB[4] = { 0.278f, 0.479f, 0, 0 };
static float mColorA[4] = { 0.25f, 0.45f, 1, 1 };
static float mColorB[4] = { 0.25f, 0.45f, 1, 1 };
static float mPhase;
static UINT mRandom;

static void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result)) ExitProcess((UINT)result);
}

/* Same LCG and 24-bit samples in [-1, 1) as the original intro. */
static float GetUniform(void)
{
    mRandom = 1664525u * mRandom + 1013904223u;
    return (float)(mRandom >> 8) * (1.0f / 8388608.0f) - 1.0f;
}

static void Interpolate(float *result, const float *a, const float *b)
{
    UINT n;
    for (n = 0; n < 4; ++n) result[n] = (1.0f - mPhase) * a[n] + mPhase * b[n];
}

static void UpdateAnimation(UINT delta)
{
    UINT n;
    /* Mu and color always share the same approximately 20-second interval. */
    mPhase += 0.01f * ((int)delta * (1.0f / 200.0f));
    if (mPhase >= 1.0f) {
        mPhase = 0;
        for (n = 0; n < 4; ++n) {
            mMuA[n] = mMuB[n];
            mMuB[n] = GetUniform();
            mColorA[n] = mColorB[n];
        }
        do {
            for (n = 0; n < 3; ++n) mColorB[n] = GetUniform();
        } while (mColorB[0] < 0 && mColorB[1] < 0 && mColorB[2] < 0);
    }
    Interpolate(mConstants.mu, mMuA, mMuB);
    Interpolate(mConstants.diffuse, mColorA, mColorB);
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
            sizeof(MainConstantBuffer), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0
        };
        ID3D11Device *device;
        ID3D11DeviceContext *context;
        IDXGISwapChain *swapChain;
        ID3D11Texture2D *texture;
        ID3D11Buffer *constants;
        ID3D11UnorderedAccessView *output;
        ID3D11ComputeShader *shader;
        DWORD start, elapsed, previous = 0;
        MSG message;
        HWND window = CreateWindowW(L"edit", NULL, WS_POPUP | WS_VISIBLE,
            WINPOSX, WINPOSY, WINWIDTH, WINHEIGHT, NULL, NULL, NULL, NULL);
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
        ThrowIfFailed(device->lpVtbl->CreateComputeShader(device, g_CS_QJulia4D, sizeof(g_CS_QJulia4D), NULL, &shader));
        context->lpVtbl->CSSetShader(context, shader, NULL, 0);
        context->lpVtbl->CSSetConstantBuffers(context, 0, 1, &constants);

        start = GetTickCount();
        mRandom = start;
        for (;;) {
            while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT || message.message == WM_CLOSE) ExitProcess(0);
                DispatchMessageW(&message);
            }
            elapsed = GetTickCount() - start;
            if (elapsed > 30000 || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) break;
            UpdateAnimation(elapsed - previous);
            previous = elapsed;
            /* Copy from ordinary CPU memory. No mapped pointer or per-frame
             * writes to invariant matrix/viewport fields are required. */
            context->lpVtbl->UpdateSubresource(context, (ID3D11Resource *)constants, 0, NULL, &mConstants, 0, 0);
            context->lpVtbl->CSSetUnorderedAccessViews(context, 0, 1, &output, NULL);
            context->lpVtbl->Dispatch(context, (WINWIDTH + 3) / 4, (WINHEIGHT + 63) / 64, 1);
            ThrowIfFailed(swapChain->lpVtbl->Present(swapChain, 0, 0));
        }
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
#endif
    }
    /* Release relies on process teardown, as in the other tiny demos. */
    ExitProcess(0);
}
