/* Julia by Jan Vlietinck, HLSL port by Wolfgang Engel. Native Vulkan backend. */
#define WINWIDTH 1280
#define WINHEIGHT 720
#define DEMO_DURATION 30000
#define DEMO_TITLE L"Julia 4D - Vulkan"
#define DEMO_COMPUTE
#include "../Common/Compute.h"
#include "../Common/Julia.h"
#include "qjulia4D.spirv.h"
static VK_COMPUTE julia;
static void demoInitialize(void)
{
    vkComputeInitialize(sizeof(mConstants));
    julia=vkCompute(spv_CS_QJulia4D,sizeof(spv_CS_QJulia4D),"CS_QJulia4D",0,0);
    mRandom=GetTickCount();
}
static void demoFrame(UINT elapsed,UINT delta)
{
    (void)elapsed; UpdateAnimation(delta); memcpy(vkUniform.mapped,&mConstants,sizeof(mConstants));
    vkComputeBegin(); vkDispatch(&julia,4,64); vkComputeEnd();
}
static void demoShutdown(void) { vkDeleteCompute(&julia); vkDeleteBuffer(&vkUniform); vkDeleteImage(&vkOutput); }
#include "../Common/Entry.h"
