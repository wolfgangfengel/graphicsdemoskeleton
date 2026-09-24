#include <stdio.h>
#include <windows.h>
static LARGE_INTEGER frequency,started,ended;
static unsigned frames;
static DWORD WINAPI benchWait(HANDLE handle,DWORD timeout)
{
    DWORD result=WaitForSingleObject(handle,timeout);
    ++frames;
    if(frames==60) QueryPerformanceCounter(&started);
    if(frames==660) {
        QueryPerformanceCounter(&ended);
        printf("600 frames: %.3f s, %.2f FPS\n",(double)(ended.QuadPart-started.QuadPart)/frequency.QuadPart,
            600.*frequency.QuadPart/(ended.QuadPart-started.QuadPart));fflush(stdout);
        PostQuitMessage(0);
    }
    return result;
}
static HWND WINAPI benchForeground(void) {return 0;}
#define WaitForSingleObject benchWait
#define GetForegroundWindow benchForeground
#include "Window.c"
int __cdecl main(void)
{
    QueryPerformanceFrequency(&frequency);
    winmain();
    return 1;
}
