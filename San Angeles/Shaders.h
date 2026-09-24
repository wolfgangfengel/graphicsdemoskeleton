/* Shared startup compilation for the two native San Angeles renderers.
 * Shader sources derive from Jetro Lauha's demo; see license-BSD.txt.
 */
struct ID3D10Effect; /* Forward declaration missing from the SDK's C prototype. */
#include <d3dcompiler.h>
#include "ShaderSource.h"

static ID3DBlob *saCompileShader(const char *entry,const char *target)
{
    ID3DBlob *bytecode;
    HRESULT result=D3DCompile(shaderSource,sizeof(shaderSource)-1,0,0,0,entry,target,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&bytecode,0);
    if (FAILED(result)) ExitProcess((UINT)result);
    return bytecode;
}
