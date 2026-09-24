/* Backend supplies hInitialize(HWND), hRender(), hPresent(), hShutdown(). */
__declspec(naked) void __cdecl winmain(void)
{
    __asm {
        push ebp
        mov ebp,esp
        sub esp,__LOCAL_SIZE
    }
    {
        MSG message;
        HWND window=CreateWindowW(L"edit",L"Himalayas",WS_POPUP|WS_VISIBLE,200,200,WINWIDTH,WINHEIGHT,0,0,0,0);
        if(!window) ExitProcess(1);
        hInitialize(window);
        for(;;) {
            while(PeekMessageW(&message,0,0,0,PM_REMOVE)) {
                if(message.message==WM_QUIT || message.message==WM_CLOSE ||
                    (message.hwnd==window && message.message==WM_KEYDOWN && message.wParam==VK_ESCAPE)) goto finished;
                if(message.hwnd==window && hInput(&message)) continue;
                DispatchMessageW(&message);
            }
            if(IsIconic(window)) {hStopLook(0);Sleep(10);continue;}
            hBeginFrame();
            hUpdateCamera(window,hFrameDelta);
            hConstants[2]=hFrameTime; hConstants[3]=(float)hFrameNumber;
            hRender(); hPresent(); ++hFrameNumber;
            hEndFrame();
        }
finished:
        hEndControls();
#ifdef _DEBUG
        hShutdown(); DestroyWindow(window);
#endif
        ExitProcess(0);
    }
}
