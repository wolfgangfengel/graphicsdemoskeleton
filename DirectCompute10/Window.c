////////////////////////////////////////////////////////////////////////
//
// Port of Jan Vlietnick's Julia 4D demo 
//
// by Wolfgang Engel 
//
// Last time modified: 10/26/2011 
//
///////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <Windows.h>
#include <sal.h>
#include <rpcsal.h>

#define DEFINE_GUIDW(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
//DEFINE_GUIDW(IID_ID3D10Texture2D,0x9B7E4C04,0x342C,0x4106,0xA1,0x9F,0x4F,0x27,0x04,0xF6,0x89,0xF0);
DEFINE_GUIDW(IID_ID3D11Texture2D,0x6f15aaf2,0xd208,0x4e89,0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c);

#include <d3d11.h>
#include <d3dx11.h>

#include <D3Dcompiler.h>

// define the size of the window
#define WINWIDTH 800 
#define WINHEIGHT 600
#define WINPOSX 200 
#define WINPOSY 200

// makes the applicaton behave well with windows
// allows to remove some system calls to reduce size
#define WELLBEHAVIOUR
#define COMPILENWRITEOUTSHADERS
#define SHORTENTRYPOINT

#define DIRECTX101


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
    return (u) * 2.328306435454494e-10 * 2.0;
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

//        b[ 0 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
//        b[ 1 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
//        b[ 2 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
//        b[ 3 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
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
//    v[ 0 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
//    v[ 1 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
//    v[ 2 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
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


#if !defined(SHORTENTRYPOINT)

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
	ID3D11UnorderedAccessView*  pComputeOutput = NULL;  // compute output


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

	float timer = 0;

	// timer global variables
	DWORD		StartTime;
	DWORD		CurrentTime;

	// keep track if the game loop is still running
	BOOL		BRunning;

	// the most simple window
	HWND hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE, WINPOSX, WINPOSY, WINWIDTH, WINHEIGHT, 0, 0, 0, 0);

	// don't show the cursor
	ShowCursor(FALSE);

	const static DXGI_SWAP_CHAIN_DESC sd = {{WINWIDTH, WINHEIGHT, {60, 1}, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0}, {1, 0}, DXGI_USAGE_RENDER_TARGET_OUTPUT, 1, NULL, TRUE, DXGI_SWAP_EFFECT_SEQUENTIAL, 0};

	//
	DXGI_SWAP_CHAIN_DESC temp;
	temp = sd;
	temp.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_SHADER_INPUT;
	temp.OutputWindow = hWnd;

#if defined(DIRECTX101)
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
#endif

	D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL, 
			D3D11_CREATE_DEVICE_DEBUG,
#if defined(DIRECTX101)
			&featureLevel,
			1,
#else
			NULL,
			0,
#endif
			D3D11_SDK_VERSION,
			&temp,
			&pSwapChain,
			&pd3dDevice,
			NULL,
			&pImmediateContext);


  	DXGI_SWAP_CHAIN_DESC sdtemp;
  	pSwapChain->lpVtbl->GetDesc(pSwapChain, &sdtemp);


	//
    // Create constant buffer
	//
	typedef struct
	{
 		float diffuse[4]; // diffuse shading color
		float mu[4];    // quaternion julia parameter
  		float epsilon;  // detail julia
		int c_width;      // view port size
		int c_height;
		int selfShadow;  // selfshadowing on or off 
		float orientation[4*4]; // rotation matrix
		float zoom;
	} QJulia4DConstants;

    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = ((sizeof( QJulia4DConstants ) + 15)/16)*16; // must be multiple of 16 bytes
    pd3dDevice->lpVtbl->CreateBuffer(pd3dDevice, &Desc, NULL, &pcbFractal);

#if !defined(DIRECTX101)
	// get access to the back buffer via a texture
  	ID3D11Texture2D* pTexture;
  	pSwapChain->lpVtbl->GetBuffer(pSwapChain, 0, (REFIID ) &IID_ID3D11Texture2D, ( LPVOID* )&pTexture );

    // create shader unordered access view on back buffer for compute shader to write into texture
   	pd3dDevice->lpVtbl->CreateUnorderedAccessView(pd3dDevice,(ID3D11Resource*)pTexture, NULL, &pComputeOutput );
#else
	ID3D11RenderTargetView *pRenderTargetView;

	// Create a back buffer render target, get a view on it to clear it later
	ID3D11Texture2D *pBackBuffer;
	pSwapChain->lpVtbl->GetBuffer( pSwapChain, 0, (REFIID ) &IID_ID3D11Texture2D, (LPVOID*)&(pBackBuffer) ) ;
	pd3dDevice->lpVtbl->CreateRenderTargetView( pd3dDevice, (ID3D11Resource*)pBackBuffer, NULL, &pRenderTargetView );
	pImmediateContext->lpVtbl->OMSetRenderTargets( pImmediateContext, 1, &pRenderTargetView, NULL );

	const static D3D11_VIEWPORT vp = {0, 0, WINWIDTH, WINHEIGHT, 0, 1}; 
	pImmediateContext->lpVtbl->RSSetViewports( pImmediateContext, 1, &vp );
#endif


#if defined(DIRECTX101)
	ID3D11Buffer*				pStructuredBuffer;
	ID3D11UnorderedAccessView*  pComputeOutputUAV = NULL;  // compute output
	ID3D11ShaderResourceView*	pComputeShaderSRV = NULL; 

	// 
	// structured buffer
	//
	struct BufferStruct
	{
		UINT color[4];
	};

	D3D11_BUFFER_DESC sbDesc;
	sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	sbDesc.CPUAccessFlags = 0;
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbDesc.StructureByteStride = 16; // sizeof(BufferStruct);
	sbDesc.ByteWidth = 16 * ((WINWIDTH + 3) / 4) * ((WINHEIGHT + 63) / 64);
	sbDesc.Usage = D3D11_USAGE_DEFAULT;
	pd3dDevice->lpVtbl->CreateBuffer(pd3dDevice, &sbDesc, NULL, &pStructuredBuffer);

	// 
	// Unordered access view on structured buffer
	//
	D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
	sbUAVDesc.Buffer.FirstElement = 0;
	sbUAVDesc.Buffer.Flags = 0;
	sbUAVDesc.Buffer.NumElements = ((WINWIDTH + 3) / 4) * ((WINHEIGHT + 63) / 64);
	sbUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	pd3dDevice->lpVtbl->CreateUnorderedAccessView(pd3dDevice, (ID3D11Resource *)pStructuredBuffer, &sbUAVDesc, &pComputeOutputUAV);

	// 
	// shader resource view on structured buffer
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC sbSRVDesc;
	sbSRVDesc.Buffer.ElementOffset = 0;
	sbSRVDesc.Buffer.ElementWidth = 16; // sizeof(BufferStruct);
	sbSRVDesc.Buffer.FirstElement = 0;
	sbSRVDesc.Buffer.NumElements = ((WINWIDTH + 3) / 4) * ((WINHEIGHT + 63) / 64);
	sbSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	sbSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	pd3dDevice->lpVtbl->CreateShaderResourceView(pd3dDevice, (ID3D11Resource *) pStructuredBuffer, &sbSRVDesc, &pComputeShaderSRV);
#endif


	//
	// compile a compute shader
	//
	ID3DBlob *pByteCodeBlob = NULL;
	ID3DBlob *pErrorBlob = NULL;
//	ID3DBlob *pCompressedByteCodeBlob = NULL;
	ID3D11ComputeShader *pCompiledComputeShader = NULL;

#ifdef COMPILENWRITEOUTSHADERS
 	 HRESULT hr = D3DX11CompileFromFile( "qjulia4D.hlsl", NULL, NULL, "CS_QJulia4D", "cs_4_0", 0, 0, NULL, &pByteCodeBlob, &pErrorBlob, NULL);
	
	// seems to require DirectX 11.1  
	//HRESULT hr = D3DCompileFromFile(L"qjulia4D.hlsl", NULL, NULL, "CS_QJulia4D", "cs_5_0", NULL, NULL, &pByteCodeBlob, NULL, pErrorBlob );
/*	
	char array[64];
	int length = strlen(pComputeShader);
	int test = sprintf(array, "%d", length);
	MessageBoxA(NULL, array, "Error", MB_OK | MB_ICONERROR);

	HRESULT hr = D3DCompile(pComputeShader, length, NULL, NULL, NULL, "main", "cs_5_0", 0, 0, &pByteCodeBlob, &pErrorBlob);
*/
	char* ErrMessage;

	if(hr != S_OK)
		MessageBoxA(NULL, (char *)pErrorBlob->lpVtbl->GetBufferPointer(pErrorBlob), "Error", MB_OK | MB_ICONERROR);

/*	D3D_SHADER_DATA ShaderData;
			ShaderData.pBytecode = pByteCodeBlob->lpVtbl->GetBufferPointer(pByteCodeBlob);
			ShaderData.BytecodeLength = pByteCodeBlob->lpVtbl->GetBufferSize(pByteCodeBlob);

	hr = D3DCompressShaders(1, &ShaderData, D3D_COMPRESS_SHADER_KEEP_ALL_PARTS, &pCompressedByteCodeBlob);
	if(hr != S_OK)
		MessageBoxA(NULL, "D3DCompressShaders() failed", "Error", MB_OK | MB_ICONERROR);
*/	
	// seems to require DirectX 11.1
	//D3DWriteBlobToFile(&pByteCodeBlob, "ComputeShader.sh", TRUE);
#endif


	// seems to require DirectX 11.1
	//D3DReadFileToBlob("ComputeShader.sh", &pByteCodeBlob);

	// compressed version seems to be 571 in size while the uncompressed version is 776
//	unsigned int sizeOfCompressedBlob = pCompressedByteCodeBlob->lpVtbl->GetBufferSize(pCompressedByteCodeBlob);
//	D3DDecompressShaders(pCompressedByteCodeBlob->lpVtbl->GetBufferPointer(pCompressedByteCodeBlob), sizeOfCompressedBlob, 1, 0, NULL, 0, &pByteCodeBlob, NULL);

#ifdef COMPILENWRITEOUTSHADERS
	hr =
#endif 
	pd3dDevice->lpVtbl->CreateComputeShader(pd3dDevice, pByteCodeBlob->lpVtbl->GetBufferPointer(pByteCodeBlob), pByteCodeBlob->lpVtbl->GetBufferSize(pByteCodeBlob), NULL, &pCompiledComputeShader);

#ifdef COMPILENWRITEOUTSHADERS
	if(hr != S_OK)
		MessageBoxA(NULL, "CreateComputerShader() failed", "Error", MB_OK | MB_ICONERROR);
#endif

	// setup timer 
	StartTime = GetTickCount();
	CurrentTime = 0;	

	// seed the random number generator
	SetSeed((unsigned int)GetCurrentTime());
	//inlineSrand((unsigned int)GetCurrentTime());

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
		if (CurrentTime > 30000 || GetAsyncKeyState(VK_ESCAPE)) 
			BRunning = FALSE;

		dt = CurrentTime / (1000.0 * 20);

	    UpdateMu( &MuT, MuA, MuB );
 	    Interpolate( MuC, MuT, MuA, MuB );
  
  	    UpdateColor( &ColorT, ColorA, ColorB );
   		Interpolate(ColorC, ColorT, ColorA, ColorB );

		// Fill constant buffer
		D3D11_MAPPED_SUBRESOURCE msr;
  		pImmediateContext->lpVtbl->Map(pImmediateContext,(ID3D11Resource *)pcbFractal, 0, D3D11_MAP_WRITE_DISCARD, 0,  &msr);

    		static QJulia4DConstants mc;

    		mc.c_height = (int)WINHEIGHT;
    		mc.c_width  = (int)WINWIDTH;
    		mc.diffuse[0] = ColorC[0];
    		mc.diffuse[1] = ColorC[1];
    		mc.diffuse[2] = ColorC[2];
    		mc.diffuse[3] = ColorC[3];
    		mc.epsilon = Epsilon;
    		mc.mu[0] = MuC[0];
    		mc.mu[1] = MuC[1];
    		mc.mu[2] = MuC[2];
    		mc.mu[3] = MuC[3];
 /*   		for (int j=0; j<3; j++)
    			for (int i=0; i<3; i++)
     		 mc.orientation[i + 4*j] = 0.0; //trackBall.GetRotationMatrix()(j,i);
 */			
			mc.orientation[0] = 1.0;
			mc.orientation[1] = 0.0;
			mc.orientation[2] = 0.0;
			mc.orientation[3] = 0.0;
			mc.orientation[4] = 0.0;
			mc.orientation[5] = 1.0;
			mc.orientation[6] = 0.0;
			mc.orientation[7] = 0.0;
			mc.orientation[8] = 0.0;
			mc.orientation[9] = 0.0;
			mc.orientation[10] = 1.0;
			mc.orientation[11] = 0.0;
			mc.orientation[12] = 0.0;
			mc.orientation[13] = 0.0;
			mc.orientation[14] = 0.0;
			mc.orientation[15] = 1.0;

   			mc.selfShadow = selfShadow;
    		mc.zoom = zoom;
    		*(QJulia4DConstants *)msr.pData = mc;
  		pImmediateContext->lpVtbl->Unmap(pImmediateContext, (ID3D11Resource *)pcbFractal,0);

    	// Set compute shader
    	pImmediateContext->lpVtbl->CSSetShader(pImmediateContext, pCompiledComputeShader, NULL, 0 );

    	// For CS output
#if defined(DIRECTX101)
		pImmediateContext->lpVtbl->CSSetUnorderedAccessViews(pImmediateContext, 0, 1, &pComputeOutputUAV, NULL);
#else
    	pImmediateContext->lpVtbl->CSSetUnorderedAccessViews(pImmediateContext, 0, 1, &pComputeOutput, NULL);
#endif

    	// For CS constant buffer
    	pImmediateContext->lpVtbl->CSSetConstantBuffers(pImmediateContext, 0, 1, &pcbFractal );

    	// Run the CS
    	pImmediateContext->lpVtbl->Dispatch(pImmediateContext, (WINWIDTH + 3) / 4, (WINHEIGHT + 63) / 64, 1 );

#if defined(DIRECTX101)
		//
		// to make CopyResource work it would need to be the same type of resource
		//
    	//pImmediateContext->lpVtbl->CSSetUnorderedAccessViews(pImmediateContext, 0, 1, &pComputeOutput, NULL);

		//pImmediateContext->lpVtbl->CopyResource(pImmediateContext, (ID3D11Resource *)pTexture, (ID3D11Resource *)pStructuredBuffer );

		// D3D11 on D3D10 hW: only a single UAV can be bound to a pipeline at once. 
 		// set to NULL to unbind
		ID3D11UnorderedAccessView* pNullUAV = NULL;
		pImmediateContext->lpVtbl->CSSetUnorderedAccessViews(pImmediateContext, 0, 1, &pNullUAV, NULL);

 		// draw into the backbuffer
   		static const float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
		pImmediateContext->lpVtbl->ClearRenderTargetView(pImmediateContext, pRenderTargetView, ClearColor );

		// line 1504 of HDRToneMapping example

		// to draw into the backbuffer we need a shader resource view

		// then we store the access values to read from the structured buffer in a constant buffer 
		/*
    	D3D11_MAPPED_SUBRESOURCE MappedResource;            
    	V( pd3dImmediateContext->Map( g_pcbCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    	UINT* p = (UINT*)MappedResource.pData;
    	p[0] = dwWidth;
    	p[1] = dwHeight;
    	pd3dImmediateContext->Unmap( g_pcbCS, 0 );
    	ID3D11Buffer* ppCB[1] = { g_pcbCS };
    	pd3dImmediateContext->PSSetConstantBuffers( g_iCBPSBind, 1, ppCB );
		*/

		// then we draw a quad and copy from the constant buffer into the back buffer

		// this might be the shader who does that
		/*
		StructuredBuffer<float4> buffer : register( t0 );

		struct QuadVS_Output
		{
		    float4 Pos : SV_POSITION;              
		    float2 Tex : TEXCOORD0;
		};

		cbuffer cbPS : register( b0 )
		{
		    uint4    g_param;   
		};

		float4 PSDump( QuadVS_Output Input ) : SV_TARGET
		{
		   // To calculate the buffer offset, it is natural to use the screen space coordinates,
		   // Input.Pos is the screen space coordinates of the pixel being written 
	 	   return buffer[ (Input.Pos.x - 0.5) + (Input.Pos.y - 0.5) * g_param.x ];	
		}
		*/

#endif
		// make it visible
		pSwapChain->lpVtbl->Present( pSwapChain, 0, 0 );
	}

	// release all D3D device related resources
#if defined(WELLBEHAVIOUR)
	    pImmediateContext->lpVtbl->ClearState(pImmediateContext);
	    pd3dDevice->lpVtbl->Release(pd3dDevice);
	    pSwapChain->lpVtbl->Release(pSwapChain);	
#if !defined(DIRECTX101) 
	    pTexture->lpVtbl->Release(pTexture);	
		pComputeOutput->lpVtbl->Release(pComputeOutput);
#endif
		pByteCodeBlob->lpVtbl->Release(pByteCodeBlob);
		//pErrorBlob->lpVtbl->Release(pErrorBlob);
		//pCompressedByteCodeBlob->lpVtbl->Release(pCompressedByteCodeBlob);
    	pcbFractal->lpVtbl->Release(pcbFractal);

#if defined(DIRECTX101) 
		pStructuredBuffer->lpVtbl->Release(pStructuredBuffer);
		pComputeOutputUAV->lpVtbl->Release(pComputeOutputUAV);
		pComputeShaderSRV->lpVtbl->Release(pComputeShaderSRV);
#endif

#endif // defined(WELLBEHAVIOUR)

#if !defined(SHORTENTRYPOINT)
    return (int) msg.wParam;
#else
	}

	ExitProcess(0);
#endif
}
