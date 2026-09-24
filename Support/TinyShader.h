#ifndef TINY_SHADER_H
#define TINY_SHADER_H

struct ID3D10Effect; /* Forward declaration missing from the SDK's C prototype. */
#include <d3dcompiler.h>

/* Compile once before starting the playback clock; no CRT or external HLSL. */
static ID3DBlob *tinyCompileShader(const char *source,SIZE_T length,const char *entry,const char *target,UINT flags2)
{
    ID3DBlob *bytecode;
    HRESULT result=D3DCompile(source,length,0,0,0,entry,target,D3DCOMPILE_OPTIMIZATION_LEVEL3,flags2,&bytecode,0);
    if (FAILED(result)) ExitProcess((UINT)result);
    return bytecode;
}
#define tinyCompile(entry,target,flags2) tinyCompileShader(source_##entry,sizeof(source_##entry)-1,#entry,target,flags2)

#endif
