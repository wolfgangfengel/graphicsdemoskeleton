#include "Vulkan.h"
static VK_IMAGE vkOutput;
static VK_BUFFER vkUniform;
typedef struct { VkPipeline pipeline; VkPipelineLayout layout; VkDescriptorSetLayout bindings; VkDescriptorPool pool; VkDescriptorSet set; } VK_COMPUTE;
static VK_COMPUTE vkCompute(const void *code,UINT bytes,const char *entry,VK_BUFFER *intermediate,int writeBuffer)
{
    VK_COMPUTE result={0};
    VkDescriptorSetLayoutBinding bindings[3]={{0}};
    static VkDescriptorSetLayoutCreateInfo setLayout={VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    static VkPipelineLayoutCreateInfo layout={VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    static VkComputePipelineCreateInfo pipeline={VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    VkDescriptorPoolSize sizes[3]={{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1}};
    static VkDescriptorPoolCreateInfo pool={VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    static VkDescriptorSetAllocateInfo allocate={VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    VkDescriptorImageInfo image={0};
    VkDescriptorBufferInfo uniform={0},buffer={0};
    VkWriteDescriptorSet writes[3]={{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}};
    UINT count=intermediate?3:2,n;
    bindings[0].binding=0; bindings[0].descriptorType=writeBuffer?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].binding=2; bindings[1].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].binding=1; bindings[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    for(n=0;n<count;++n) { bindings[n].descriptorCount=1; bindings[n].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT; }
    setLayout.bindingCount=count; setLayout.pBindings=bindings;
    vkCheck(vkCreateDescriptorSetLayout(vkDevice,&setLayout,0,&result.bindings));
    layout.setLayoutCount=1; layout.pSetLayouts=&result.bindings;
    vkCheck(vkCreatePipelineLayout(vkDevice,&layout,0,&result.layout));
    pipeline.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; pipeline.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline.stage.module=vkShader(code,bytes); pipeline.stage.pName=entry; pipeline.layout=result.layout;
    vkCheck(vkCreateComputePipelines(vkDevice,0,1,&pipeline,0,&result.pipeline)); vkDestroyShaderModule(vkDevice,pipeline.stage.module,0);
    pool.maxSets=1; pool.poolSizeCount=3; pool.pPoolSizes=sizes;
    vkCheck(vkCreateDescriptorPool(vkDevice,&pool,0,&result.pool));
    allocate.descriptorPool=result.pool; allocate.descriptorSetCount=1; allocate.pSetLayouts=&result.bindings;
    vkCheck(vkAllocateDescriptorSets(vkDevice,&allocate,&result.set));
    image.imageView=vkOutput.view; image.imageLayout=VK_IMAGE_LAYOUT_GENERAL;
    uniform.buffer=vkUniform.handle; uniform.range=VK_WHOLE_SIZE;
    if(intermediate) { buffer.buffer=intermediate->handle; buffer.range=VK_WHOLE_SIZE; }
    for(n=0;n<count;++n) { writes[n].dstSet=result.set; writes[n].dstBinding=bindings[n].binding; writes[n].descriptorCount=1; writes[n].descriptorType=bindings[n].descriptorType; }
    writes[0].pImageInfo=&image; writes[0].pBufferInfo=&buffer;
    writes[1].pBufferInfo=&uniform; writes[2].pBufferInfo=&buffer;
    vkUpdateDescriptorSets(vkDevice,count,writes,0,0);
    return result;
}
static void vkComputeInitialize(UINT constantBytes)
{
    vkOutput=vkImage(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
    vkUniform=vkBuffer(constantBytes,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,1);
}
static void vkDispatch(const VK_COMPUTE *compute,UINT groupX,UINT groupY)
{
    vkCmdBindPipeline(vkCommands,VK_PIPELINE_BIND_POINT_COMPUTE,compute->pipeline);
    vkCmdBindDescriptorSets(vkCommands,VK_PIPELINE_BIND_POINT_COMPUTE,compute->layout,0,1,&compute->set,0,0);
    vkCmdDispatch(vkCommands,(WINWIDTH+groupX-1)/groupX,(WINHEIGHT+groupY-1)/groupY,1);
}
static void vkComputeBegin(void) { vkBarrier(vkOutput.handle,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL,0,VK_ACCESS_SHADER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT); }
static void vkComputeEnd(void) { vkBarrier(vkOutput.handle,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_SHADER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT); }
static void vkDeleteCompute(VK_COMPUTE *compute)
{
    vkDestroyPipeline(vkDevice,compute->pipeline,0); vkDestroyPipelineLayout(vkDevice,compute->layout,0);
    vkDestroyDescriptorPool(vkDevice,compute->pool,0); vkDestroyDescriptorSetLayout(vkDevice,compute->bindings,0);
}
