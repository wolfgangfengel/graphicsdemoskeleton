#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VULKAN_TEST_MESSAGE(s) do{puts(s);fflush(stdout);}while(0)
#include "Window.c"
#ifdef PROBE_DX11
#include <d3d11sdklayers.h>
#elif defined(PROBE_DX12)
#include <d3d12sdklayers.h>
#endif
static void save(const char *name,const unsigned char *pixels,UINT stride)
{
    FILE *file=fopen(name,"wb");UINT y;
    if(!file)exit(10);
    for(y=0;y<WINHEIGHT;++y) fwrite(pixels+y*stride,4,WINWIDTH,file);
    fclose(file);
}
int __cdecl main(int argc,char **argv)
{
    UINT f; DWORD fixedTime=argc>1?(DWORD)atoi(argv[1]):0; int route=argc>2?atoi(argv[2]):0;
    HWND window=CreateWindowW(L"edit",L"Himalayas capture",WS_POPUP,0,0,WINWIDTH,WINHEIGHT,0,0,0,0);
#ifdef PROBE_VULKAN
    VkCommandBufferBeginInfo begin={VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VkSubmitInfo submit={VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VK_BUFFER readback;
    VkBufferImageCopy copy={0};
    vkWindow=window;vkInitialize();demoInitialize();
    readback=vkBuffer(WINWIDTH*WINHEIGHT*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,1);
    copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;copy.imageSubresource.layerCount=1;
    copy.imageExtent.width=WINWIDTH;copy.imageExtent.height=WINHEIGHT;copy.imageExtent.depth=1;
    submit.commandBufferCount=1;submit.pCommandBuffers=&vkCommands;
#else
    hInitialize(window);
#endif
    if(route==1) {hMoveCamera(1,1,1,20);hRotateCamera(140,-60);}
    if(route==2) {hConstants[5]=350;hConstants[8]=-.6f;}
    if(route==3) {hConstants[5]=240;hConstants[8]=0;}
    if(route==4) {hConstants[5]=62;hConstants[8]=0;}
    for(f=0;f<64;++f) {
        if(route==5) {
            if(f>=8 && f<40) {hMoveCamera(1,1,.5f,.5f);hRotateCamera(6,-1);}
            if(f==48) hResetCamera();
        }
#ifdef PROBE_MOUSE
        if(f==8 || f==32 || f==48) {
            MSG mouse={0};
            mouse.hwnd=window;mouse.message=f==8?WM_LBUTTONDOWN:f==32?WM_MOUSEMOVE:WM_LBUTTONUP;
            mouse.wParam=MK_LBUTTON;mouse.lParam=MAKELPARAM(f==8?600:840,280);
            hInput(&mouse);
            if((f<48 && hConstants[6]<=0) || (f==48 && hConstants[6]>=0))return 3;
        }
#endif
#ifdef PROBE_VULKAN
        vkCheck(vkResetCommandPool(vkDevice,vkCommandPool,0));vkCheck(vkBeginCommandBuffer(vkCommands,&begin));
        hFrameTime=(float)fixedTime*.001f;hFrameDelta=0;demoFrame(fixedTime,0);
        vkCmdCopyImageToBuffer(vkCommands,vkOutput.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.handle,1,&copy);
        {
            VkMemoryBarrier barrier={VK_STRUCTURE_TYPE_MEMORY_BARRIER};
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
            vkCmdPipelineBarrier(vkCommands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,0,0,0);
        }
        vkCheck(vkEndCommandBuffer(vkCommands));vkCheck(vkQueueSubmit(vkQueue,1,&submit,0));vkCheck(vkQueueWaitIdle(vkQueue));
        if(f==0)save("first.rgba",readback.mapped,WINWIDTH*4);
#else
        hConstants[2]=(float)fixedTime*.001f;hConstants[3]=(float)f;
        hRender();hPresent();++hFrameNumber;
#endif
    }
#ifdef PROBE_DX11
    {
        ID3D11Texture2D *readback;
        D3D11_TEXTURE2D_DESC desc;
        D3D11_MAPPED_SUBRESOURCE mapped;
        ID3D11InfoQueue *messages;
        UINT64 n,count;
        {
            UINT textureIndex,x,y,bad=0;
            for(textureIndex=2;textureIndex<=4;textureIndex+=2) {
                hTextures[textureIndex]->lpVtbl->GetDesc(hTextures[textureIndex],&desc);
                desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
                hCheck(hDevice->lpVtbl->CreateTexture2D(hDevice,&desc,0,&readback));
                hContext->lpVtbl->CopyResource(hContext,(ID3D11Resource *)readback,(ID3D11Resource *)hTextures[textureIndex]);
                hCheck(hContext->lpVtbl->Map(hContext,(ID3D11Resource *)readback,0,D3D11_MAP_READ,0,&mapped));
                for(y=2;y<WINHEIGHT;++y)for(x=0;x<WINWIDTH*4;++x) {
                    unsigned bits=((unsigned *)((char *)mapped.pData+y*mapped.RowPitch))[x];
                    if((bits&0x7f800000)==0x7f800000)++bad;
                }
                hContext->lpVtbl->Unmap(hContext,(ID3D11Resource *)readback,0);readback->lpVtbl->Release(readback);
            }
            printf("Non-finite terrain/cloud values: %u\n",bad);if(bad)return 4;
        }
        hTextures[6]->lpVtbl->GetDesc(hTextures[6],&desc);
        desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        hCheck(hDevice->lpVtbl->CreateTexture2D(hDevice,&desc,0,&readback));
        hContext->lpVtbl->CopyResource(hContext,(ID3D11Resource *)readback,(ID3D11Resource *)hTextures[6]);
        hCheck(hContext->lpVtbl->Map(hContext,(ID3D11Resource *)readback,0,D3D11_MAP_READ,0,&mapped));
        save("frame.rgba",mapped.pData,mapped.RowPitch);
        hContext->lpVtbl->Unmap(hContext,(ID3D11Resource *)readback,0);readback->lpVtbl->Release(readback);
        hCheck(hDevice->lpVtbl->QueryInterface(hDevice,&IID_ID3D11InfoQueue,(void **)&messages));
        count=messages->lpVtbl->GetNumStoredMessagesAllowedByRetrievalFilter(messages);
        for(n=0;n<count;++n) {
            SIZE_T size=0;D3D11_MESSAGE *m;
            messages->lpVtbl->GetMessage(messages,n,0,&size);m=malloc(size);messages->lpVtbl->GetMessage(messages,n,m,&size);
            if(m->Severity<=D3D11_MESSAGE_SEVERITY_WARNING){puts(m->pDescription);return 2;}free(m);
        }
        messages->lpVtbl->Release(messages);
    }
#elif defined(PROBE_DX12)
    {
        static const D3D12_HEAP_PROPERTIES heap={D3D12_HEAP_TYPE_READBACK,0,0,1,1};
        static const D3D12_RESOURCE_DESC desc={D3D12_RESOURCE_DIMENSION_BUFFER,0,WINWIDTH*WINHEIGHT*4,1,1,1,DXGI_FORMAT_UNKNOWN,{1,0},D3D12_TEXTURE_LAYOUT_ROW_MAJOR,0};
        ID3D12Resource *readback;
        D3D12_TEXTURE_COPY_LOCATION src={0},dst={0};
        ID3D12InfoQueue *messages;
        UINT64 n,count;void *pixels;
        hCheck(hDevice->lpVtbl->CreateCommittedResource(hDevice,&heap,0,&desc,D3D12_RESOURCE_STATE_COPY_DEST,0,&IID_ID3D12Resource,(void **)&readback));
        hCheck(hAllocator->lpVtbl->Reset(hAllocator));hCheck(hCommands->lpVtbl->Reset(hCommands,hAllocator,0));
        src.pResource=hTextures[6];src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.pResource=readback;dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dst.PlacedFootprint.Footprint.Format=DXGI_FORMAT_R8G8B8A8_UNORM;dst.PlacedFootprint.Footprint.Width=WINWIDTH;dst.PlacedFootprint.Footprint.Height=WINHEIGHT;
        dst.PlacedFootprint.Footprint.Depth=1;dst.PlacedFootprint.Footprint.RowPitch=WINWIDTH*4;
        hTransition(hTextures[6],D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        hCommands->lpVtbl->CopyTextureRegion(hCommands,&dst,0,0,0,&src,0);
        hTransition(hTextures[6],D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        hCheck(hCommands->lpVtbl->Close(hCommands));hQueue->lpVtbl->ExecuteCommandLists(hQueue,1,(ID3D12CommandList *const *)&hCommands);
        hCheck(hQueue->lpVtbl->Signal(hQueue,hFence,++hFenceValue));hCheck(hFence->lpVtbl->SetEventOnCompletion(hFence,hFenceValue,0));
        hCheck(readback->lpVtbl->Map(readback,0,0,&pixels));save("frame.rgba",pixels,WINWIDTH*4);readback->lpVtbl->Unmap(readback,0,0);readback->lpVtbl->Release(readback);
        hCheck(hDevice->lpVtbl->QueryInterface(hDevice,&IID_ID3D12InfoQueue,(void **)&messages));
        count=messages->lpVtbl->GetNumStoredMessagesAllowedByRetrievalFilter(messages);
        for(n=0;n<count;++n) {
            SIZE_T size=0;D3D12_MESSAGE *m;
            messages->lpVtbl->GetMessage(messages,n,0,&size);m=malloc(size);messages->lpVtbl->GetMessage(messages,n,m,&size);
            if(m->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){puts(m->pDescription);return 2;}free(m);
        }
        messages->lpVtbl->Release(messages);
    }
#else
    save("frame.rgba",readback.mapped,WINWIDTH*4);vkDeleteBuffer(&readback);demoShutdown();
    printf("Vulkan validation active: %d; %u errors, %u warnings\n",vkMessenger!=0,vkErrors,vkWarnings);
    if(!vkMessenger || vkErrors || vkWarnings)return 2;
#endif
#ifndef PROBE_VULKAN
    hShutdown();puts("D3D validation: 0 errors, 0 warnings");
#endif
    DestroyWindow(window);puts("Captured 64 sequential frames");return 0;
}
