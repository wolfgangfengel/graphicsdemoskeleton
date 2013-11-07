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
