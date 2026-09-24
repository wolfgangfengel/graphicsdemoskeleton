#include "Vulkan.h"
static VK_IMAGE vkOutput, vkDepth;
static VkRenderPass vkRenderPass;
static VkFramebuffer vkFramebuffer;
static VkPipelineLayout vkGraphicsLayout;
static VkPipeline vkModelPipeline,vkFloorPipeline;
static void vkGraphicsInitialize(const void *vertex,UINT vertexBytes,const void *pixel,UINT pixelBytes,UINT constantBytes,int city)
{
    VkAttachmentDescription attachments[2]={{0}};
    VkAttachmentReference color={0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth={1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass={0};
    VkSubpassDependency dependency={0};
    static VkRenderPassCreateInfo renderPass={VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    VkImageView views[2];
    static VkFramebufferCreateInfo framebuffer={VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    VkPushConstantRange push={VK_SHADER_STAGE_VERTEX_BIT,0,constantBytes};
    static VkPipelineLayoutCreateInfo layout={VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    VkPipelineShaderStageCreateInfo stages[2]={{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
    VkVertexInputBindingDescription binding={0,28,VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributes[3]={{0,0,VK_FORMAT_R32G32B32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32_SFLOAT,12},{2,0,VK_FORMAT_R8G8B8A8_UNORM,24}};
    static VkPipelineVertexInputStateCreateInfo input={VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    static VkPipelineInputAssemblyStateCreateInfo assembly={VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    VkViewport viewport={0,0,WINWIDTH,WINHEIGHT,0,1};
    VkRect2D scissor={{0,0},{WINWIDTH,WINHEIGHT}};
    static VkPipelineViewportStateCreateInfo viewportState={VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    static VkPipelineRasterizationStateCreateInfo raster={VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    static VkPipelineMultisampleStateCreateInfo samples={VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    static VkPipelineDepthStencilStateCreateInfo depthState={VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState blend={0};
    static VkPipelineColorBlendStateCreateInfo blendState={VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    static VkGraphicsPipelineCreateInfo pipeline={VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    vkOutput=vkImage(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
    views[0]=vkOutput.view;
    attachments[0].format=VK_FORMAT_R8G8B8A8_UNORM; attachments[0].samples=VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR; attachments[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE; attachments[0].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    if(city) {
        VkFormatProperties properties;
        VkFormat depthFormat=VK_FORMAT_D24_UNORM_S8_UINT;
        vkGetPhysicalDeviceFormatProperties(vkPhysical,depthFormat,&properties);
        /* D24 is optional in Vulkan (including on some AMD drivers). D32
         * preserves at least the original precision throughout this scene. */
        if(!(properties.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) depthFormat=VK_FORMAT_D32_SFLOAT;
        vkGetPhysicalDeviceFormatProperties(vkPhysical,depthFormat,&properties);
        vkRequire((properties.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)!=0);
        vkDepth=vkImage(depthFormat,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT);
        views[1]=vkDepth.view; attachments[1]=attachments[0]; attachments[1].format=depthFormat;
        attachments[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE; attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS; subpass.colorAttachmentCount=1; subpass.pColorAttachments=&color;
    if(city) subpass.pDepthStencilAttachment=&depth;
    dependency.srcSubpass=0; dependency.dstSubpass=VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dependency.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependency.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; dependency.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    renderPass.attachmentCount=city?2:1; renderPass.pAttachments=attachments; renderPass.subpassCount=1; renderPass.pSubpasses=&subpass;
    renderPass.dependencyCount=1; renderPass.pDependencies=&dependency;
    vkCheck(vkCreateRenderPass(vkDevice,&renderPass,0,&vkRenderPass));
    framebuffer.renderPass=vkRenderPass; framebuffer.attachmentCount=renderPass.attachmentCount; framebuffer.pAttachments=views;
    framebuffer.width=WINWIDTH; framebuffer.height=WINHEIGHT; framebuffer.layers=1;
    vkCheck(vkCreateFramebuffer(vkDevice,&framebuffer,0,&vkFramebuffer));
    layout.pushConstantRangeCount=1; layout.pPushConstantRanges=&push;
    vkCheck(vkCreatePipelineLayout(vkDevice,&layout,0,&vkGraphicsLayout));
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT; stages[0].module=vkShader(vertex,vertexBytes); stages[0].pName="VShader";
    stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module=vkShader(pixel,pixelBytes); stages[1].pName="PShader";
    if(city) { input.vertexBindingDescriptionCount=1; input.pVertexBindingDescriptions=&binding; input.vertexAttributeDescriptionCount=3; input.pVertexAttributeDescriptions=attributes; }
    assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    viewportState.viewportCount=viewportState.scissorCount=1; viewportState.pViewports=&viewport; viewportState.pScissors=&scissor;
    raster.lineWidth=1; raster.frontFace=VK_FRONT_FACE_CLOCKWISE;
    samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    depthState.depthTestEnable=depthState.depthWriteEnable=city; depthState.depthCompareOp=VK_COMPARE_OP_LESS;
    blend.colorWriteMask=15; blendState.attachmentCount=1; blendState.pAttachments=&blend;
    pipeline.stageCount=2; pipeline.pStages=stages; pipeline.pVertexInputState=&input; pipeline.pInputAssemblyState=&assembly;
    pipeline.pViewportState=&viewportState; pipeline.pRasterizationState=&raster; pipeline.pMultisampleState=&samples;
    pipeline.pDepthStencilState=&depthState; pipeline.pColorBlendState=&blendState; pipeline.layout=vkGraphicsLayout; pipeline.renderPass=vkRenderPass;
    vkCheck(vkCreateGraphicsPipelines(vkDevice,0,1,&pipeline,0,&vkModelPipeline));
    if(city) {
        depthState.depthTestEnable=depthState.depthWriteEnable=VK_FALSE;
        blend.blendEnable=VK_TRUE; blend.dstColorBlendFactor=VK_BLEND_FACTOR_SRC_COLOR; blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
        vkCheck(vkCreateGraphicsPipelines(vkDevice,0,1,&pipeline,0,&vkFloorPipeline));
    }
    vkDestroyShaderModule(vkDevice,stages[0].module,0); vkDestroyShaderModule(vkDevice,stages[1].module,0);
}
static void vkGraphicsBegin(float r,float g,float b)
{
    VkClearValue clear[2]={0};
    static VkRenderPassBeginInfo info={VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    clear[0].color.float32[0]=r; clear[0].color.float32[1]=g; clear[0].color.float32[2]=b; clear[0].color.float32[3]=1;
    clear[1].depthStencil.depth=1;
    info.renderPass=vkRenderPass; info.framebuffer=vkFramebuffer; info.renderArea.extent.width=WINWIDTH; info.renderArea.extent.height=WINHEIGHT;
    info.clearValueCount=vkDepth.handle?2:1; info.pClearValues=clear;
    vkCmdBeginRenderPass(vkCommands,&info,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vkCommands,VK_PIPELINE_BIND_POINT_GRAPHICS,vkModelPipeline);
}
static void vkGraphicsShutdown(void)
{
    if(vkFloorPipeline) vkDestroyPipeline(vkDevice,vkFloorPipeline,0);
    vkDestroyPipeline(vkDevice,vkModelPipeline,0); vkDestroyPipelineLayout(vkDevice,vkGraphicsLayout,0);
    vkDestroyFramebuffer(vkDevice,vkFramebuffer,0); vkDestroyRenderPass(vkDevice,vkRenderPass,0);
    if(vkDepth.handle) vkDeleteImage(&vkDepth); vkDeleteImage(&vkOutput);
}
