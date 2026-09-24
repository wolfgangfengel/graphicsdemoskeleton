/* Wolfgang Engel's triangle: World * View * Perspective, rendered every frame. */
#define WINWIDTH 800
#define WINHEIGHT 600
#define DEMO_DURATION 3300
#define DEMO_TITLE L"Triangle - Vulkan"
#include "../Common/Graphics.h"
#include "vertex.spirv.h"
#include "pixel.spirv.h"
#include "Transform.h"
static void demoInitialize(void) { vkGraphicsInitialize(spv_VShader,sizeof(spv_VShader),spv_PShader,sizeof(spv_PShader),sizeof(mWorldViewProjection),0); }
static void demoFrame(UINT elapsed,UINT delta)
{
    (void)delta; UpdateTransform(elapsed); vkGraphicsBegin(0,0.2f,0.4f);
    vkCmdPushConstants(vkCommands,vkGraphicsLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(mWorldViewProjection),&mWorldViewProjection);
    vkCmdDraw(vkCommands,3,1,0,0); vkCmdEndRenderPass(vkCommands);
}
static void demoShutdown(void) { vkGraphicsShutdown(); }
#include "../Common/Entry.h"
