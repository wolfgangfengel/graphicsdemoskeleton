/* Julia by Jan Vlietinck; HLSL and color filters by Wolfgang Engel. */
#define WINWIDTH 1280
#define WINHEIGHT 720
#define DEMO_DURATION 30000
#define DEMO_TITLE L"PostFX color filters - Vulkan"
#define DEMO_COMPUTE
#include "../Common/Compute.h"
#include "../Common/Julia.h"
#include "qjulia4D.spirv.h"
#include "ColorFilters.spirv.h"
/* Keep the original 160-byte packoffset layout, including unused padding. */
typedef struct {
    UINT height,width; float epsilon; int selfShadow;
    float diffuse[4],mu[4],orientation[16],zoom,saturation;
    float correct[3],add[3],contrast[3],padding;
} POST_CONSTANTS;
static VK_BUFFER intermediate;
static VK_COMPUTE julia,filter;
static void demoInitialize(void)
{
    vkComputeInitialize(sizeof(POST_CONSTANTS));
    intermediate=vkBuffer(WINWIDTH*WINHEIGHT*16,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,0);
    julia=vkCompute(spv_CS_QJulia4D,sizeof(spv_CS_QJulia4D),"CS_QJulia4D",&intermediate,1);
    filter=vkCompute(spv_PostFX,sizeof(spv_PostFX),"PostFX",&intermediate,0);
    mRandom=GetTickCount();
}
static void demoFrame(UINT elapsed,UINT delta)
{
    POST_CONSTANTS *constants=vkUniform.mapped;
    VkMemoryBarrier barrier={VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    (void)elapsed; UpdateAnimation(delta);
    constants->height=WINHEIGHT; constants->width=WINWIDTH; constants->epsilon=mConstants.epsilon; constants->selfShadow=1;
    memcpy(constants->diffuse,mConstants.diffuse,16); memcpy(constants->mu,mConstants.mu,16); memcpy(constants->orientation,mConstants.orientation,64);
    constants->zoom=constants->saturation=1; constants->correct[0]=1.5f; constants->correct[1]=constants->correct[2]=0.5f;
    memset(constants->add,0,sizeof(constants->add)); memset(constants->contrast,0,sizeof(constants->contrast));
    vkComputeBegin(); vkDispatch(&julia,16,16);
    barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT; barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(vkCommands,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,0,0,0);
    vkDispatch(&filter,16,16); vkComputeEnd();
}
static void demoShutdown(void) { vkDeleteCompute(&julia); vkDeleteCompute(&filter); vkDeleteBuffer(&intermediate); vkDeleteBuffer(&vkUniform); vkDeleteImage(&vkOutput); }
#include "../Common/Entry.h"
