/* San Angeles by Jetro Lauha, 2004-2005. See ../../San Angeles/license-BSD.txt. */
#define WINWIDTH 1280
#define WINHEIGHT 720
#define DEMO_TITLE L"San Angeles - Vulkan"
#include "../Common/Graphics.h"
#define SA_EXTERNAL_VERTICES
#include "../../San Angeles/Scene.h"
#define DEMO_DURATION SA_RUN_LENGTH
#include "vertex.spirv.h"
#include "pixel.spirv.h"
static SA_CONSTANTS constants;
static VK_BUFFER vertices;
#include "Draw.h"
static void demoInitialize(void)
{
    vkGraphicsInitialize(spv_VShader,sizeof(spv_VShader),spv_PShader,sizeof(spv_PShader),sizeof(constants),1);
    vertices=vkBuffer(SA_VERTEX_LIMIT*sizeof(SA_VERTEX),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,1);
    saVertices=vertices.mapped; saCreateScene(); constants.right[3]=2.41421356237f*WINHEIGHT/WINWIDTH;
}
static void demoFrame(UINT elapsed,UINT delta)
{
    static const VkDeviceSize offset=0;
    (void)delta; saCamera(elapsed,&constants);
    vkGraphicsBegin(0.1f*constants.eye[3],0.2f*constants.eye[3],0.3f*constants.eye[3]);
    vkCmdBindVertexBuffers(vkCommands,0,1,&vertices.handle,&offset);
    drawModels(elapsed,-1);
    vkCmdBindPipeline(vkCommands,VK_PIPELINE_BIND_POINT_GRAPHICS,vkFloorPipeline);
    drawMesh(SUPERSHAPE_COUNT,0,0,0,1,0,1,0);
    vkCmdBindPipeline(vkCommands,VK_PIPELINE_BIND_POINT_GRAPHICS,vkModelPipeline);
    drawModels(elapsed,1); vkCmdEndRenderPass(vkCommands);
}
static void demoShutdown(void) { vkDeleteBuffer(&vertices); vkGraphicsShutdown(); }
#include "../Common/Entry.h"
