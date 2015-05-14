	//
	// compile a compute shader
	//
	ID3DBlob *pByteCodeBlob = NULL;
	ID3DBlob *pErrorBlob = NULL;
	ID3DBlob *pCompressedByteCodeBlob = NULL;
	ID3D11ComputeShader *pCompiledComputeShader = NULL;


	D3D_SHADER_DATA ShaderData;

	// seems to require DirectX 11.1  
	HRESULT hr = D3DCompileFromFile(L"qjulia4D.hlsl", NULL, NULL, "CS_QJulia4D", "cs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pByteCodeBlob, &pErrorBlob);

	if(hr != S_OK)
		MessageBoxA(NULL, (char *)pErrorBlob->lpVtbl->GetBufferPointer(pErrorBlob), "Error", MB_OK | MB_ICONERROR);

	ShaderData.pBytecode = pByteCodeBlob->lpVtbl->GetBufferPointer(pByteCodeBlob);
	ShaderData.BytecodeLength = pByteCodeBlob->lpVtbl->GetBufferSize(pByteCodeBlob);

	hr = D3DCompressShaders(1, &ShaderData, D3D_COMPRESS_SHADER_KEEP_ALL_PARTS, &pCompressedByteCodeBlob);
	if(hr != S_OK)
		MessageBoxA(NULL, "D3DCompressShaders() failed", "Error", MB_OK | MB_ICONERROR);
	
	// seems to require DirectX 11.1
	D3DWriteBlobToFile(pCompressedByteCodeBlob, L"ComputeShader.sh", TRUE);

	
//
// The code above creates the binary ... the code below reads it
//	

	// seems to require DirectX 11.1
	D3DReadFileToBlob(L"ComputeShader.sh", &pCompressedByteCodeBlob);

	// compressed version seems to be 571 in size while the uncompressed version is 776
	unsigned int sizeOfCompressedBlob = pCompressedByteCodeBlob->lpVtbl->GetBufferSize(pCompressedByteCodeBlob);
	D3DDecompressShaders(pCompressedByteCodeBlob->lpVtbl->GetBufferPointer(pCompressedByteCodeBlob), sizeOfCompressedBlob, 1, 0, NULL, 0, &pByteCodeBlob, NULL);


	hr = pd3dDevice->lpVtbl->CreateComputeShader(pd3dDevice, pByteCodeBlob->lpVtbl->GetBufferPointer(pByteCodeBlob), pByteCodeBlob->lpVtbl->GetBufferSize(pByteCodeBlob), NULL, &pCompiledComputeShader);

	if(hr != S_OK)
		MessageBoxA(NULL, "CreateComputerShader() failed", "Error", MB_OK | MB_ICONERROR);
		
		
Other idea (too big):

const char *ColorFilter =
"StructuredBuffer<float4> C:register(t0);"
"RWTexture2D<float4> c:register(u0);\n"
"#define THREADX 16\n"
"#define THREADY 16\ncbuffer cbCS:register(b0){uint c_height:packoffset(c0.x);uint c_width:packoffset(c0.y);float Saturation:packoffset(c7.y);float ColorCorrectRed:packoffset(c7.z);float ColorCorrectGreen:packoffset(c7.w);float ColorCorrectBlue:packoffset(c8.x);float ColorAddRed:packoffset(c8.y);float ColorAddGreen:packoffset(c8.z);float ColorAddBlue:packoffset(c8.w);float3 Contrast:packoffset(c9);};"
"groupshared float3 f[THREADX*THREADY];"
"[numthreads(THREADX,THREADY,1)]"
"void t(uint3 t:SV_GroupID,uint3 n:SV_DispatchThreadID,uint3 e:SV_GroupThreadID,uint T:SV_GroupIndex)"
"{"
"uint u=n.x+n.y*c_width;"
"f[T]=C[u].xyz;"
"GroupMemoryBarrierWithGroupSync();"
"float3 D;"
"if(u<c_width*c_height-1)"
"{"
"float3 l=f[T],d=l-Contrast*(l-1.f)*l*(l-.5f);"
"float S=dot(d,float3(.2126,.7152,.0722));"
"float3 h=lerp(S.xxx,d,Saturation);"
"D=h*float3(ColorCorrectRed,ColorCorrectGreen,ColorCorrectBlue)*float3(2.f,2.f,2.f)+float3(ColorAddRed,ColorAddGreen,ColorAddBlue);"
"}"
"c[n.xy]=float4(D,1.);"
"}";


	//
	// compile the compute shaders
	//
	ID3D11ComputeShader *pCompiledComputeShader;
	ID3D11ComputeShader *pCompiledPostFXComputeShader;

#if defined(_DEBUG)
	hr =
#endif
	pd3dDevice->lpVtbl->CreateComputeShader(pd3dDevice, g_CS_QJulia4D, sizeof(g_CS_QJulia4D), NULL, &pCompiledComputeShader);

	ID3DBlob *pByteCodeBlob = NULL;
	ID3DBlob *pErrorBlob = NULL;

#if defined(_DEBUG)
	hr =
#endif
	// seems to require DirectX 11.1  
	//D3DCompileFromFile(ColorFilter, NULL, NULL, "t", "cs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pByteCodeBlob, &pErrorBlob);
	D3DCompile2(ColorFilter, sizeof(char) * 1044, NULL, NULL, NULL, "t", "cs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, 0, NULL, NULL, &pByteCodeBlob, &pErrorBlob);


#if defined(_DEBUG)
	if (hr != S_OK)
		MessageBoxA(NULL, (char *)pErrorBlob->lpVtbl->GetBufferPointer(pErrorBlob), "Error", MB_OK | MB_ICONERROR);
#endif
		
