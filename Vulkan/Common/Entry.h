/* Include after demoInitialize(), demoFrame(elapsed,delta), demoShutdown(). */
static int vkMessages(void)
{
    MSG message;
    while(PeekMessageW(&message,0,0,0,PM_REMOVE)) {
        if(message.message==WM_QUIT || message.message==WM_CLOSE || (message.hwnd==vkWindow && message.message==WM_KEYDOWN && message.wParam==VK_ESCAPE)) return 0;
#ifdef DEMO_MESSAGE
        if(message.hwnd==vkWindow) DEMO_MESSAGE(message);
#endif
        if(message.hwnd==vkWindow && (message.message==WM_LBUTTONDOWN || (message.message==WM_MOUSEMOVE && (message.wParam&MK_LBUTTON)))) {
            int x=(short)LOWORD(message.lParam); vkMouseX=x<0?0:x>WINWIDTH?WINWIDTH:x;
        }
        DispatchMessageW(&message);
    }
    return 1;
}
__declspec(naked) void __cdecl winmain(void)
{
    __asm {
        push ebp
        mov ebp,esp
        sub esp,__LOCAL_SIZE
    }
    {
        DWORD start,previous;
        int recreate=1;
        static VkCommandBufferBeginInfo begin={VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        VkPipelineStageFlags waitStage=VK_PIPELINE_STAGE_TRANSFER_BIT;
        static VkSubmitInfo submit={VK_STRUCTURE_TYPE_SUBMIT_INFO};
        static VkPresentInfoKHR present={VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkWindow=CreateWindowW(L"edit",DEMO_TITLE,WS_POPUP|WS_VISIBLE,200,200,WINWIDTH,WINHEIGHT,0,0,0,0);
        vkRequire(vkWindow!=0);
        vkInitialize(); demoInitialize();
        submit.waitSemaphoreCount=1; submit.pWaitSemaphores=&vkAcquired; submit.pWaitDstStageMask=&waitStage;
        submit.commandBufferCount=1; submit.pCommandBuffers=&vkCommands; submit.signalSemaphoreCount=1;
        present.waitSemaphoreCount=1; present.swapchainCount=1; present.pSwapchains=&vkSwapchain; present.pImageIndices=&vkImageIndex;
        start=previous=GetTickCount();
        while(vkMessages()) {
            DWORD now=GetTickCount(),elapsed=now-start;
            VkResult result;
#if DEMO_DURATION > 0
            if(elapsed>DEMO_DURATION) break;
#endif
            if(IsIconic(vkWindow)) {
#ifdef DEMO_PAUSE
                DEMO_PAUSE();
#endif
                Sleep(10); continue;
            }
            if(recreate) {
                vkCheck(vkDeviceWaitIdle(vkDevice)); vkDeleteSwapchain();
                if(!vkMakeSwapchain()) { Sleep(10); continue; }
                recreate=0;
            }
#ifdef DEMO_BEGIN_FRAME
            DEMO_BEGIN_FRAME();
#endif
            vkCheck(vkWaitForFences(vkDevice,1,&vkFence,VK_TRUE,~(uint64_t)0));
            result=vkAcquireNextImageKHR(vkDevice,vkSwapchain,~(uint64_t)0,vkAcquired,0,&vkImageIndex);
            if(result==VK_ERROR_OUT_OF_DATE_KHR) { recreate=1; continue; }
            vkCheck(result); if(result==VK_SUBOPTIMAL_KHR) recreate=1;
            vkCheck(vkResetCommandPool(vkDevice,vkCommandPool,0)); vkCheck(vkBeginCommandBuffer(vkCommands,&begin));
            demoFrame(elapsed,now-previous); previous=now;
            /* Chain the layout transition to the acquire semaphore's TRANSFER
             * wait. TOP_OF_PIPE here would permit transition before acquisition. */
            vkBarrier(vkImages[vkImageIndex],VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
#ifdef DEMO_SKELETON
            {
                static const VkClearColorValue clear={{0,0.2f,0.4f,1}};
                vkCmdClearColorImage(vkCommands,vkImages[vkImageIndex],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&clear,1,&vkColorRange);
            }
#else
            {
                VkImageBlit blit={0};
                blit.srcSubresource.aspectMask=blit.dstSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.layerCount=blit.dstSubresource.layerCount=1;
                blit.srcOffsets[1].x=WINWIDTH; blit.srcOffsets[1].y=WINHEIGHT; blit.srcOffsets[1].z=1;
                blit.dstOffsets[1].x=vkExtent.width; blit.dstOffsets[1].y=vkExtent.height; blit.dstOffsets[1].z=1;
                vkCmdBlitImage(vkCommands,vkOutput.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,vkImages[vkImageIndex],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,VK_FILTER_NEAREST);
            }
#endif
            vkBarrier(vkImages[vkImageIndex],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,VK_ACCESS_TRANSFER_WRITE_BIT,0,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
            vkCheck(vkEndCommandBuffer(vkCommands));
            submit.pSignalSemaphores=&vkPresented[vkImageIndex]; present.pWaitSemaphores=submit.pSignalSemaphores;
            vkCheck(vkResetFences(vkDevice,1,&vkFence)); vkCheck(vkQueueSubmit(vkQueue,1,&submit,vkFence));
            result=vkQueuePresentKHR(vkQueue,&present);
            if(result==VK_ERROR_OUT_OF_DATE_KHR || result==VK_SUBOPTIMAL_KHR) recreate=1; else vkCheck(result);
#ifdef DEMO_END_FRAME
            DEMO_END_FRAME();
#endif
        }
        vkCheck(vkDeviceWaitIdle(vkDevice));
#ifdef DEMO_END
        DEMO_END();
#endif
#ifdef _DEBUG
        demoShutdown(); vkDeleteSwapchain(); vkDestroyFence(vkDevice,vkFence,0); vkDestroySemaphore(vkDevice,vkAcquired,0);
        vkDestroyCommandPool(vkDevice,vkCommandPool,0); vkDestroyDevice(vkDevice,0); vkDestroySurfaceKHR(vkInstance,vkSurface,0);
        if(vkMessenger) vkDestroyMessenger(vkInstance,vkMessenger,0);
        vkDestroyInstance(vkInstance,0); DestroyWindow(vkWindow);
        if(vkErrors) ExitProcess(2);
#endif
    }
    ExitProcess(0);
}
