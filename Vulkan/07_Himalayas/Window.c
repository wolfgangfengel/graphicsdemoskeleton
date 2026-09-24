// Himalayas by Reinder Nijhoff (2018). See ../../Himalayas/LICENSE.txt.
#define WINWIDTH 1280
#define WINHEIGHT 720
#define DEMO_DURATION 0
#define DEMO_FAST_PRESENT
#define DEMO_TITLE L"Himalayas - Vulkan"
#include "../Common/Vulkan.h"
#include "../../Himalayas/Input.h"
#include "A.spirv.h"
#include "B.spirv.h"
#include "C.spirv.h"
#include "D.spirv.h"
#include "Image.spirv.h"
static VK_IMAGE hTextures[7],vkOutput;
static VK_BUFFER hUniform;
static VkSampler hSamplers[2];
static VkDescriptorSetLayout hBindings;
static VkPipelineLayout hLayout;
static VkDescriptorPool hPool;
static VkDescriptorSet hSets[10];
static VkPipeline hShaders[5];
static const void *hSources[]={spv_CS_A,spv_CS_B,spv_CS_C,spv_CS_D,spv_CS_Image};
static const UINT hSourceBytes[]={sizeof(spv_CS_A),sizeof(spv_CS_B),sizeof(spv_CS_C),sizeof(spv_CS_D),sizeof(spv_CS_Image)};
static const char *hEntryNames[]={"CS_A","CS_B","CS_C","CS_D","CS_Image"};
static void demoInitialize(void)
{
    static VkSamplerCreateInfo sampler={VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    static VkDescriptorSetLayoutBinding bindings[8];
    static VkDescriptorSetLayoutCreateInfo layout={VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    static VkPipelineLayoutCreateInfo pipelineLayout={VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    static VkDescriptorPoolSize sizes[4]={{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,10},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,40},{VK_DESCRIPTOR_TYPE_SAMPLER,20},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,10}};
    static VkDescriptorPoolCreateInfo pool={VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    static VkDescriptorSetAllocateInfo allocate={VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    static VkComputePipelineCreateInfo pipeline={VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    VkFormatProperties format;
    UINT n,p,parity,output;
    int inputs[4];
    vkGetPhysicalDeviceFormatProperties(vkPhysical,VK_FORMAT_R32G32B32A32_SFLOAT,&format);
    vkRequire((format.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)!=0);
    for(n=0;n<7;++n) hTextures[n]=vkImage(n==6?VK_FORMAT_R8G8B8A8_UNORM:VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|(n==6?VK_IMAGE_USAGE_TRANSFER_SRC_BIT:VK_IMAGE_USAGE_SAMPLED_BIT),VK_IMAGE_ASPECT_COLOR_BIT);
    vkOutput=hTextures[6];hUniform=vkBuffer(sizeof(hConstants),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,1);
    sampler.magFilter=sampler.minFilter=VK_FILTER_LINEAR;sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
    for(n=0;n<2;++n) {
        sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=n?VK_SAMPLER_ADDRESS_MODE_REPEAT:VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCheck(vkCreateSampler(vkDevice,&sampler,0,&hSamplers[n]));
    }
    for(n=0;n<8;++n) {
        bindings[n].binding=n;bindings[n].descriptorCount=1;bindings[n].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[n].descriptorType=n==0?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:n<5?VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:n<7?VK_DESCRIPTOR_TYPE_SAMPLER:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
    layout.bindingCount=8;layout.pBindings=bindings;
    vkCheck(vkCreateDescriptorSetLayout(vkDevice,&layout,0,&hBindings));
    pipelineLayout.setLayoutCount=1;pipelineLayout.pSetLayouts=&hBindings;
    vkCheck(vkCreatePipelineLayout(vkDevice,&pipelineLayout,0,&hLayout));
    pool.maxSets=10;pool.poolSizeCount=4;pool.pPoolSizes=sizes;
    vkCheck(vkCreateDescriptorPool(vkDevice,&pool,0,&hPool));
    allocate.descriptorPool=hPool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&hBindings;
    for(parity=0;parity<2;++parity) for(p=0;p<5;++p) {
        UINT set=parity*5+p;
        // Initialization is single threaded; reuse zero-initialized descriptors.
        static VkDescriptorImageInfo images[7];
        static VkDescriptorBufferInfo uniform={0,0,sizeof(hConstants)};
        static VkWriteDescriptorSet writes[8];
        uniform.buffer=hUniform.handle;
        vkCheck(vkAllocateDescriptorSets(vkDevice,&allocate,&hSets[set]));
        hPassImages(p,parity,&output,inputs);
        images[0].imageView=hTextures[output].view;
        for(n=1;n<5;++n) images[n].imageView=hTextures[inputs[n-1]<0?0:inputs[n-1]].view;
        images[5].sampler=hSamplers[0];images[6].sampler=hSamplers[1];
        for(n=0;n<8;++n) {
            if(n<7) {images[n].imageLayout=VK_IMAGE_LAYOUT_GENERAL;writes[n].pImageInfo=&images[n];}
            writes[n].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[n].dstSet=hSets[set];writes[n].dstBinding=n;
            writes[n].descriptorCount=1;writes[n].descriptorType=bindings[n].descriptorType;
        }
        writes[7].pBufferInfo=&uniform;
        vkUpdateDescriptorSets(vkDevice,8,writes,0,0);
    }
    pipeline.layout=hLayout;pipeline.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;pipeline.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;
    for(n=0;n<5;++n) {
        pipeline.stage.module=vkShader(hSources[n],hSourceBytes[n]);pipeline.stage.pName=hEntryNames[n];
        vkCheck(vkCreateComputePipelines(vkDevice,0,1,&pipeline,0,&hShaders[n]));
        vkDestroyShaderModule(vkDevice,pipeline.stage.module,0);
    }
}
static void demoFrame(DWORD elapsed,DWORD delta)
{
    static const VkClearColorValue zero={{0}};
    UINT pass,n,output;
    int inputs[4];
    (void)elapsed;(void)delta;
    hUpdateCamera(vkWindow,hFrameDelta);
    hConstants[2]=hFrameTime;hConstants[3]=(float)hFrameNumber;
    memcpy(hUniform.mapped,hConstants,sizeof(hConstants));
    if(!hFrameNumber) for(n=0;n<7;++n) {
        vkBarrier(hTextures[n].handle,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdClearColorImage(vkCommands,hTextures[n].handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&zero,1,&vkColorRange);
        vkBarrier(hTextures[n].handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }
    else vkBarrier(vkOutput.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    for(pass=hFrameNumber?2:0;pass<5;++pass) {
        UINT set=(hFrameNumber&1)*5+pass;
        hPassImages(pass,hFrameNumber&1,&output,inputs);
        if(output!=6) vkBarrier(hTextures[output].handle,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_GENERAL,VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_SHADER_WRITE_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        vkCmdBindPipeline(vkCommands,VK_PIPELINE_BIND_POINT_COMPUTE,hShaders[pass]);
        vkCmdBindDescriptorSets(vkCommands,VK_PIPELINE_BIND_POINT_COMPUTE,hLayout,0,1,&hSets[set],0,0);
        vkCmdDispatch(vkCommands,(WINWIDTH+7)/8,(WINHEIGHT+7)/8,1);
        if(output!=6) vkBarrier(hTextures[output].handle,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_GENERAL,VK_ACCESS_SHADER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }
    vkBarrier(vkOutput.handle,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_SHADER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    ++hFrameNumber;
}
static void demoShutdown(void)
{
    UINT n;
    for(n=0;n<5;++n) vkDestroyPipeline(vkDevice,hShaders[n],0);
    vkDestroyDescriptorPool(vkDevice,hPool,0);vkDestroyPipelineLayout(vkDevice,hLayout,0);vkDestroyDescriptorSetLayout(vkDevice,hBindings,0);
    for(n=0;n<2;++n) vkDestroySampler(vkDevice,hSamplers[n],0);
    for(n=0;n<7;++n) vkDeleteImage(&hTextures[n]);
    vkDeleteBuffer(&hUniform);
}
#define DEMO_MESSAGE(message) if(hInput(&(message))) continue
#define DEMO_BEGIN_FRAME() hBeginFrame()
#define DEMO_END_FRAME() hEndFrame()
#define DEMO_PAUSE() hStopLook(0)
#define DEMO_END() hEndControls()
#include "../Common/Entry.h"
