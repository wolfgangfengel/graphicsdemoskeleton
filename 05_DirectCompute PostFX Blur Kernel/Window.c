////////////////////////////////////////////////////////////////////////
//
// Port of Jan Vlietnick's Julia 4D demo 
//
// by Wolfgang Engel 
//
// Last time modified: 01/01/2014
//
///////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <Windows.h>
#include <sal.h>
#include <rpcsal.h>

#define DEFINE_GUIDW(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
DEFINE_GUIDW(IID_ID3D11Texture2D,0x6f15aaf2,0xd208,0x4e89,0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c);

#include <d3d11.h>
#include <d3dcompiler.h>

// Macros
// Macros are error-prone because they rely on textual substitution and do not perform type-checking.
#define CLAMP(n,min,max)                        ((n < min) ? min : (n > max) ? max : n)
// #define Distance(a,b)                           sqrtf((a-b) * (a-b))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
//#define CEIL_DIV(x,y) (((x) + (y) - 1) / (y))
#define CEIL(VARIABLE) ( (VARIABLE - (int)VARIABLE)==0 ? (int)VARIABLE : (int)VARIABLE+1 )

typedef unsigned int uint32;
typedef unsigned char unit8;

// define the size of the window
#define THREADSX 16			// number of threads in the thread group used in the compute shader
#define THREADSY 16			// number of threads in the thread group used in the compute shader
#define WINDOWWIDTH 1024  
#define WINDOWHEIGHT 768 

#define WINWIDTH ((((WINDOWWIDTH + THREADSX - 1) / THREADSX) * THREADSX))	// multiply of ThreadsX 
#define WINHEIGHT ((((WINDOWHEIGHT + THREADSY - 1) / THREADSY) * THREADSY)) // multiply of ThreadsY

#define WINPOSX 50 
#define WINPOSY 50

// makes the applicaton behave well with windows
// allows to remove some system calls to reduce size
#define WELLBEHAVIOUR

// for the blur kernel filter
#define DOF_BLUR_KERNEL_RADIUS_MAX 16
#define DOF_BLUR_KERNEL_RADIUS 8
#define RUN_SIZE	128 	//	Pixels to process per line per kernel invocation
#define RUN_LINES	2  		//	Lines to process per kernel invocation


#include "qjulia4D.sh"
#include "BlurKernel.sh"

//
// Random number generator
// see http://www.codeproject.com/KB/recipes/SimpleRNG.aspx

// These values are not magical, just the default values Marsaglia used.
// Any pair of unsigned integers should be fine.
static unsigned int m_w = 521288629;
//static unsigned int m_z = 362436069;
#define MZ ((36969 * (362436069 & 65535) + (362436069 >> 16)) << 16)

static void SetSeed(unsigned int u)
{
	m_w = u;
}

// This is the heart of the generator.
// It uses George Marsaglia's MWC algorithm to produce an unsigned integer.
// See http://www.bobwheeler.com/statistics/Password/MarsagliaPost.txt
static unsigned int GetUint()
{
//	m_z = 36969 * (m_z & 65535) + (m_z >> 16);
	m_w = 18000 * (m_w & 65535) + (m_w >> 16);
	return (MZ) + m_w;
}

// Produce a uniform random sample from the interval (-1, 1).
// The method will not return either end point.
static float GetUniform()
{
    // 0 <= u < 2^32
    unsigned int u = GetUint();
	// The magic number below is 1/(2^32 + 2).
    // The result is strictly between 0 and 1.
	return (u) * (float) 2.328306435454494e-10 * 2.0f;
}


static void 
Interpolate( float m[4], float t, float a[4], float b[4] )
{
    int i;
    for ( i = 0; i < 4; i++ )
        m[ i ] = ( 1.0f - t ) * a[ i ] + t * b[ i ];
}


float dt; // time increment depending on frame rendering time for same animation speed independent of rendering speed

static void 
UpdateMu( float t[4], float a[4], float b[4] )
{
    *t += 0.01f *dt;
    
    if ( *t >= 1.0f )
    {
        *t = 0.0f;

        a[ 0 ] = b[ 0 ];
        a[ 1 ] = b[ 1 ];
        a[ 2 ] = b[ 2 ];
        a[ 3 ] = b[ 3 ];

        b[ 0 ] = GetUniform();
        b[ 1 ] = GetUniform();
        b[ 2 ] = GetUniform();
        b[ 3 ] = GetUniform();
    }
}

static void
RandomColor( float v[4] )
{
    do
    {
		v[ 0 ] = GetUniform();
		v[ 1 ] = GetUniform();
		v[ 2 ] = GetUniform();
    }
    while (v[0] < 0 && v[1] <0 && v[2]<0); // prevent black colors
    v[ 3 ] = 1.0f;
}

static void 
UpdateColor( float t[4], float a[4], float b[4] )
{
    *t += 0.01f *dt;
   
    if ( *t >= 1.0f )
    {
        *t = 0.0f;

        a[ 0 ] = b[ 0 ];
        a[ 1 ] = b[ 1 ];
        a[ 2 ] = b[ 2 ];
        a[ 3 ] = b[ 3 ];

        RandomColor(b);
    }
}

typedef struct 
{
	float	KernelWeights[DOF_BLUR_KERNEL_RADIUS_MAX + 1];
}DOF_BUFFER;

// this is a memcpy that is not very fast
void memcopy32(void* dest, void* src, int size)
{
	unit8 *pdest = (unit8*)dest;
	unit8 *psrc = (unit8*)src;

	int loops = (size / sizeof(uint32));
	for (int index = 0; index < loops; ++index)
	{
		*((uint32*)pdest) = *((uint32*)psrc);
		pdest += sizeof(uint32);
		psrc += sizeof(uint32);
	}
/*
	loops = (size % sizeof(uint32));
	for (int index = 0; index < loops; ++index)
	{
		*pdest = *psrc;
		++pdest;
		++psrc;
	}
*/
}

// http://stackoverflow.com/questions/7824239/exp-function-using-c
double my_exp(double x)
{
	double sum = 1.0 + x;
	double term = x;                 // term for k = 1 is just x
	for (int k = 2; k < 50; k++)
	{
		term = term * x / (double) k; // term[k] = term[k-1] * x / k
		sum = sum + term;
	}
	return sum;
}

/**
* Evaluates a normal distribution PDF at given X.
* This function misses the math for scaling the result (faster, not needed if the resulting values are renormalized).
* @param X - The X to evaluate the PDF at.
* @param Mean - The normal distribution's mean.
* @param Variance - The normal distribution's variance.
* @return The value of the normal distribution at X. (unscaled)
*/
static float NormalDistributionUnscaled(float X,float Mean,float Variance)
{
//	return exp(-((X - Mean) * (X - Mean)) / (2.0 * Variance));
	return (float)my_exp(-((X - Mean) * (X - Mean)) / (2.0 * Variance));
}

//	
static void CalculateWeights(float KernelRadius, DOF_BUFFER *buffer)
{
	const unsigned int DELTA = 1;

	float ClampedKernelRadius = CLAMP(KernelRadius, DELTA, DOF_BLUR_KERNEL_RADIUS_MAX);
	INT IntegerKernelRadius = MIN(CEIL(ClampedKernelRadius), DOF_BLUR_KERNEL_RADIUS_MAX);

	// smallest IntegerKernelRadius will be 1

	float WeightSum = 0.0f;
	for (INT SampleIndex = 0; SampleIndex <= IntegerKernelRadius; ++SampleIndex)
	{
		float Weight = NormalDistributionUnscaled((float) SampleIndex, (float)0, (float) ClampedKernelRadius);

		buffer->KernelWeights[SampleIndex] = Weight;
		//	Igor: All the samples with non-0 index contribute to the total sum twice as [i] and [-i]
		WeightSum += SampleIndex ? Weight * 2 : Weight;
	}

	// Normalize blur weights.
	float InvWeightSum = 1.0f / (WeightSum);
	for (INT SampleIndex = 0; SampleIndex <= IntegerKernelRadius; ++SampleIndex)
	{
		buffer->KernelWeights[SampleIndex] = buffer->KernelWeights[SampleIndex] * InvWeightSum;
	}
}


#if 0 

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
	// D3D 11 device variables
	ID3D11Device *pd3dDevice;
	IDXGISwapChain *pSwapChain;
	ID3D11DeviceContext *pImmediateContext;

	ID3D11Buffer*		    	pcbFractal;      // constant buffer
	ID3D11UnorderedAccessView*  pUAVBackbuffer;  // output into back buffer
	ID3D11UnorderedAccessView*  pUAVTempTexture;  // output into texture 2D
	ID3D11ShaderResourceView*	pSRVTempTexture;  // SRV for temporary texture
	ID3D11ShaderResourceView*	pSRVBackBuffer; // SRV for back buffer
	ID3D11Texture2D *pTempTexture;

	static D3D11_BUFFER_DESC sbDesc;
	static D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
	static D3D11_SHADER_RESOURCE_VIEW_DESC sbSRVDesc;
	static D3D11_BUFFER_DESC Desc;					// constant buffer
	static D3D11_TEXTURE2D_DESC TextureDesc;

	//
	// compile the compute shaders
	//
	ID3D11ComputeShader *pCompiledComputeShader;
	ID3D11ComputeShader *pCompiledCSFilterComputeShader;
	//ID3D11ComputeShader *pCompiledCSFilterYComputeShader;

	static DOF_BUFFER DOFBuf;

	static float gSaturation = 1.0f;

	static float Epsilon                    = 0.003f;
	static float ColorT                     = 0.0f;
	static float ColorA[4]                  = { 0.25f, 0.45f, 1.0f, 1.0f };
	static float ColorB[4]                  = { 0.25f, 0.45f, 1.0f, 1.0f };
	static float ColorC[4]                  = { 0.25f, 0.45f, 1.0f, 1.0f };

	static float MuT                        = 0.0f;
	static float MuA[4]                     = { -.278f, -.479f, 0.0f, 0.0f };
	static float MuB[4]                     = { 0.278f, 0.479f, 0.0f, 0.0f };
	static float MuC[4]                     = { -.278f, -.479f, -.231f, .235f };

	BOOL selfShadow = TRUE;
	float zoom = 1.0f;

	// timer global variables
	DWORD StartTime;
	static DWORD CurrentTime;

	// keep track if the game loop is still running
	static BOOL BStopRunning;

	// the most simple window
	HWND hWnd = CreateWindow(L"edit", 0, WS_POPUP | WS_VISIBLE, WINPOSX, WINPOSY, WINWIDTH, WINHEIGHT, 0, 0, 0, 0);

	// don't show the cursor
	ShowCursor(FALSE);

	const static DXGI_SWAP_CHAIN_DESC sd = {{WINWIDTH, WINHEIGHT, {60, 1},  DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED, DXGI_MODE_SCALING_UNSPECIFIED }, {1, 0}, DXGI_USAGE_RENDER_TARGET_OUTPUT, 1, NULL, TRUE, DXGI_SWAP_EFFECT_SEQUENTIAL, 0};

	//
	DXGI_SWAP_CHAIN_DESC temp;
	temp = sd;
	temp.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_SHADER_INPUT;
	temp.OutputWindow = hWnd;

 	D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL, 
			D3D11_CREATE_DEVICE_DEBUG,
			NULL,
			0,
			D3D11_SDK_VERSION,
			&temp,
			&pSwapChain,
			&pd3dDevice,
			NULL,
			&pImmediateContext);


  	DXGI_SWAP_CHAIN_DESC sdtemp;
  	pSwapChain->lpVtbl->GetDesc(pSwapChain, &sdtemp);
	
	// get access to the back buffer via a texture
  	ID3D11Texture2D* pTexture;
  	pSwapChain->lpVtbl->GetBuffer(pSwapChain, 0, (REFIID ) &IID_ID3D11Texture2D, ( LPVOID* )&pTexture );

	//
    // Create constant buffer
	//
	typedef struct
	{
		// Julia 4D constants
		unsigned int c_height;
		unsigned int c_width;      // view port size
		float epsilon;  // detail julia
		int selfShadow;  // selfshadowing on or off 
		float diffuse[4]; // diffuse shading color
		float mu[4];    // quaternion julia parameter
		float orientation[4*4]; // rotation matrix
		float zoom[4]; // with three dummy values to 
		// for the DOF weights
		// unfortunately D3D run-time will allocate with
		// float KernelWeights[DOF_BLUR_KERNEL_RADIUS_MAX + 1];
		// 16 x float4 + 1 x float value
		// with the following it will allocate 17 x float4 but the indexing will work as expected
		// kind of a waste of memory
		float KernelWeights[DOF_BLUR_KERNEL_RADIUS_MAX + 1][4];
	} MainConstantBuffer;

#if defined(_DEBUG)
	HRESULT hr; // track a few return statements
#endif

	// constant buffer for Julia4D
	Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.ByteWidth = ((sizeof(MainConstantBuffer)+15) / 16) * 16; // must be multiple of 16 bytes // 13 x 16 bytes
#if defined(_DEBUG)
	hr =
#endif
		pd3dDevice->lpVtbl->CreateBuffer(pd3dDevice, &Desc, NULL, &pcbFractal);

#if defined(_DEBUG)
	if (hr != S_OK)
		MessageBoxA(NULL, "Julia4D constant buffer failed", "Error", MB_OK | MB_ICONERROR);
#endif

	TextureDesc.Width = WINWIDTH;
	TextureDesc.Height = WINHEIGHT;
	TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TextureDesc.MipLevels = 1;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	TextureDesc.ArraySize = 1;
//	TextureDesc.MiscFlags = 0;

#if defined(_DEBUG)
	hr =
#endif
	pd3dDevice->lpVtbl->CreateTexture2D(pd3dDevice, &TextureDesc, NULL, &pTempTexture);

#if defined(_DEBUG)
	if (hr != S_OK)
		MessageBoxA(NULL, "Texture creation failed", "Error", MB_OK | MB_ICONERROR);
#endif

	// get UAV on the temp texture ...
#if defined(_DEBUG)
	hr =
#endif
		pd3dDevice->lpVtbl->CreateUnorderedAccessView(pd3dDevice, (ID3D11Resource*) pTempTexture, NULL, &pUAVTempTexture);

#if defined(_DEBUG)
	if (hr != S_OK)
		MessageBoxA(NULL, "UA View creation failed", "Error", MB_OK | MB_ICONERROR);
#endif

	// SRV on temp texture
	sbSRVDesc.Format = TextureDesc.Format;
	sbSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sbSRVDesc.Texture2D.MipLevels = 1;

#if defined(_DEBUG)
	hr =
#endif
		pd3dDevice->lpVtbl->CreateShaderResourceView(pd3dDevice, (ID3D11Resource *) pTempTexture, &sbSRVDesc, &pSRVTempTexture);

#if defined(_DEBUG)
	if (hr != S_OK)
		MessageBoxA(NULL, "SRV creation failed", "Error", MB_OK | MB_ICONERROR);
#endif
	
	// back buffer UAV
	// create shader unordered access view on back buffer for compute shader to write into texture
	pd3dDevice->lpVtbl->CreateUnorderedAccessView(pd3dDevice, (ID3D11Resource*) pTexture, NULL, &pUAVBackbuffer);

	// SRV on backbuffer
	sbSRVDesc.Format = TextureDesc.Format;
	sbSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sbSRVDesc.Texture2D.MipLevels = 1;

#if defined(_DEBUG)
	hr =
#endif
		pd3dDevice->lpVtbl->CreateShaderResourceView(pd3dDevice, (ID3D11Resource *) pTexture, &sbSRVDesc, &pSRVBackBuffer);

#if defined(_DEBUG)
	if (hr != S_OK)
		MessageBoxA(NULL, "SRV creation failed", "Error", MB_OK | MB_ICONERROR);
#endif

	CalculateWeights(DOF_BLUR_KERNEL_RADIUS, &DOFBuf);

#if defined(_DEBUG)
	hr =
#endif
		pd3dDevice->lpVtbl->CreateComputeShader(pd3dDevice, g_CS_QJulia4D, sizeof(g_CS_QJulia4D), NULL, &pCompiledComputeShader);
#if defined(_DEBUG)
	hr =
#endif
		pd3dDevice->lpVtbl->CreateComputeShader(pd3dDevice, g_CSFilter, sizeof(g_CSFilter), NULL, &pCompiledCSFilterComputeShader);

#if defined(_DEBUG)
	if (hr != S_OK)
		MessageBoxA(NULL, "CreateComputerShader() failed", "Error", MB_OK | MB_ICONERROR);
#endif

	// setup timer 
	StartTime = GetTickCount();

	// seed the random number generator
	SetSeed((unsigned int)GetCurrentTime());

	// set the game loop to running by default
	MSG msg;

	while (!BStopRunning)
	{
#if defined(WELLBEHAVIOUR)
		// Just remove the message
		PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE);
#endif
		// Calculate the current demo time
		CurrentTime = GetTickCount() - StartTime;

		// go out of game loop and shutdown
		if (CurrentTime > 30000 
#if defined(WELLBEHAVIOUR) 
			|| GetAsyncKeyState(VK_ESCAPE)
#endif
			)
			BStopRunning = TRUE;

		dt = CurrentTime / (20000.0f);

	    UpdateMu( &MuT, MuA, MuB );
 	    Interpolate( MuC, MuT, MuA, MuB );
  
  	    UpdateColor( &ColorT, ColorA, ColorB );
   		Interpolate(ColorC, ColorT, ColorA, ColorB );

		static MainConstantBuffer mc;

		// this is a continous constant buffer
		// that means each value is aligned in the buffer one after each other without any spaces
		// the layout need to be in the same order as the constant buffer struct in the shader
		mc.c_height = WINHEIGHT;
		mc.c_width = WINWIDTH;
		mc.epsilon = Epsilon;
		mc.selfShadow = selfShadow;
		mc.diffuse[0] = ColorC[0];
		mc.diffuse[1] = ColorC[1];
		mc.diffuse[2] = ColorC[2];
		mc.diffuse[3] = ColorC[3];
		mc.mu[0] = MuC[0];
		mc.mu[1] = MuC[1];
		mc.mu[2] = MuC[2];
		mc.mu[3] = MuC[3];
		mc.orientation[0] = 1.0;
		//		mc->orientation[1] = 0.0;
		//		mc->orientation[2] = 0.0;
		//		mc->orientation[3] = 0.0;
		//		mc->orientation[4] = 0.0;
		mc.orientation[5] = 1.0;
		//		mc->orientation[6] = 0.0;
		//		mc->orientation[7] = 0.0;
		//		mc->orientation[8] = 0.0;
		//		mc->orientation[9] = 0.0;
		mc.orientation[10] = 1.0;
		//		mc->orientation[11] = 0.0;
		//		mc->orientation[12] = 0.0;
		//		mc->orientation[13] = 0.0;
		//		mc->orientation[14] = 0.0;
		mc.orientation[15] = 1.0;
		mc.zoom[0] = zoom;
		mc.zoom[1] = 0.0;
		mc.zoom[2] = 0.0;
		mc.zoom[3] = 0.0;

		for (unsigned int i = 0; i < DOF_BLUR_KERNEL_RADIUS + 1; i++)
		{
			mc.KernelWeights[i][0] = DOFBuf.KernelWeights[i];
		}

		// Fill constant buffer
		static D3D11_MAPPED_SUBRESOURCE msr;
		pImmediateContext->lpVtbl->Map(pImmediateContext, (ID3D11Resource *)pcbFractal, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

		// copy onto GPU memory
		memcopy32(msr.pData, &mc, sizeof(mc));

  		pImmediateContext->lpVtbl->Unmap(pImmediateContext, (ID3D11Resource *)pcbFractal,0);

		//
		// run the Julia 4D code
		//
		// Set compute shader
    	pImmediateContext->lpVtbl->CSSetShader(pImmediateContext, pCompiledComputeShader, NULL, 0 );

    	// For CS output
		pImmediateContext->lpVtbl->CSSetUnorderedAccessViews(pImmediateContext, 0, 1, &pUAVBackbuffer, NULL);

    	// For CS constant buffer
		// set constanct buffer b0
    	pImmediateContext->lpVtbl->CSSetConstantBuffers(pImmediateContext, 0, 1, &pcbFractal );

    	// Run the CS
		pImmediateContext->lpVtbl->Dispatch(pImmediateContext, WINWIDTH / THREADSX, WINHEIGHT / THREADSY, 1);


		//
		// run blur kernel in X
		//
		// Set compute shader
		pImmediateContext->lpVtbl->CSSetShader(pImmediateContext, pCompiledCSFilterComputeShader, NULL, 0);

		// For CS output -> write into temporary texture
		pImmediateContext->lpVtbl->CSSetUnorderedAccessViews(pImmediateContext, 0, 1, &pUAVTempTexture, NULL);

		// For CS constant buffer
		// set constanct buffer b0
		pImmediateContext->lpVtbl->CSSetConstantBuffers(pImmediateContext, 0, 1, &pcbFractal);

		// read the backbuffer
		pImmediateContext->lpVtbl->CSSetShaderResources(pImmediateContext, 0, 1, &pSRVBackBuffer);

		// Run the CS
//		const UINT SizeXVertival = ceil(float(WINHEIGHT) / RUN_SIZE);
//		const UINT SizeYVertival = ceil(float(WINWIDTH) / RUN_LINES);

		pImmediateContext->lpVtbl->Dispatch(pImmediateContext, WINWIDTH / RUN_SIZE, WINHEIGHT / RUN_LINES, 1);

#if defined(WELLBEHAVIOUR)
		// set back the shader resource view to zero
		ID3D11ShaderResourceView* pNull = NULL;
		pImmediateContext->lpVtbl->CSSetShaderResources(pImmediateContext, 0, 1, &pNull);
#endif

		
		//
		// run blur kernel in Y
		//
		
		// switch on Y direction in constant buffer
		// Fill constant buffer
		pImmediateContext->lpVtbl->Map(pImmediateContext, (ID3D11Resource *)pcbFractal, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

		mc.zoom[1] = 1.0; // switch to y axis blurring
		// copy onto GPU memory
		memcopy32(msr.pData, &mc, sizeof(mc));

		pImmediateContext->lpVtbl->Unmap(pImmediateContext, (ID3D11Resource *)pcbFractal, 0);

		// Set compute shader
		pImmediateContext->lpVtbl->CSSetShader(pImmediateContext, pCompiledCSFilterComputeShader, NULL, 0);

		// For CS output -> write into the back buffer
		pImmediateContext->lpVtbl->CSSetUnorderedAccessViews(pImmediateContext, 0, 1, &pUAVBackbuffer, NULL);

		// For CS constant buffer
		// set constanct buffer b0
		pImmediateContext->lpVtbl->CSSetConstantBuffers(pImmediateContext, 0, 1, &pcbFractal);

		// read the temporary texture
		pImmediateContext->lpVtbl->CSSetShaderResources(pImmediateContext, 0, 1, &pSRVTempTexture);

		// Run the CS
		//		const UINT SizeXVertival = ceil(float(WINHEIGHT) / RUN_SIZE);
		//		const UINT SizeYVertival = ceil(float(WINWIDTH) / RUN_LINES);

		pImmediateContext->lpVtbl->Dispatch(pImmediateContext, WINHEIGHT / RUN_SIZE, WINWIDTH / RUN_LINES, 1);

#if defined(WELLBEHAVIOUR)
		pImmediateContext->lpVtbl->CSSetShaderResources(pImmediateContext, 0, 1, &pNull);
#endif
		
		// make it visible
		pSwapChain->lpVtbl->Present(pSwapChain, 0, 0);
	}

	// release all D3D device related resources
#if defined(WELLBEHAVIOUR)
	    pImmediateContext->lpVtbl->ClearState(pImmediateContext);
	    pd3dDevice->lpVtbl->Release(pd3dDevice);
	    pSwapChain->lpVtbl->Release(pSwapChain);	 
		pcbFractal->lpVtbl->Release(pcbFractal);
		pTexture->lpVtbl->Release(pTexture);
		pTempTexture->lpVtbl->Release(pTempTexture);
		pUAVTempTexture->lpVtbl->Release(pUAVTempTexture);
		pUAVBackbuffer->lpVtbl->Release(pUAVBackbuffer);
		pSRVTempTexture->lpVtbl->Release(pSRVTempTexture);
		pSRVBackBuffer->lpVtbl->Release(pSRVBackBuffer);
		pCompiledComputeShader->lpVtbl->Release(pCompiledComputeShader);
		pCompiledCSFilterComputeShader->lpVtbl->Release(pCompiledCSFilterComputeShader);
#endif

#if 0 
    return (int) msg.wParam;
#else
	}

	ExitProcess(0);
#endif
}
