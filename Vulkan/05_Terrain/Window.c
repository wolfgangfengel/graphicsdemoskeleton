/* Mountains by David Hoskins, 2013. See LICENSE-Mountains.txt. */
#define WINWIDTH 1280
#define WINHEIGHT 720
#define DEMO_DURATION 30000
#define DEMO_TITLE L"Mountains - Vulkan"
#define DEMO_COMPUTE
#include "../Common/Compute.h"
#include "Terrain.spirv.h"
static VK_COMPUTE terrain;
static void demoInitialize(void)
{
    vkComputeInitialize(16); terrain=vkCompute(spv_CS_Terrain,sizeof(spv_CS_Terrain),"CS_Terrain",0,0);
}
static void demoFrame(UINT elapsed,UINT delta)
{
    float *constants=vkUniform.mapped;
    (void)delta; constants[0]=(int)elapsed*0.001f; constants[1]=WINWIDTH; constants[2]=WINHEIGHT; constants[3]=(float)vkMouseX;
    vkComputeBegin(); vkDispatch(&terrain,8,8); vkComputeEnd();
}
static void demoShutdown(void) { vkDeleteCompute(&terrain); vkDeleteBuffer(&vkUniform); vkDeleteImage(&vkOutput); }
#include "../Common/Entry.h"
