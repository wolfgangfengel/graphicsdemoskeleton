/* 120 FPS target. Start the timer before GPU work, wait only for time left over.
 * High-resolution timers need Windows 10 1803; older Windows uses a normal timer.
 * No busy wait, multimedia timer-resolution change, or C runtime is required. */
static HANDLE hFrameTimer;
static double hTimerFrequency,hFrameDeadline;
static double hClockStart,hClockPrevious;
static float hFrameTime,hFrameDelta;
static void hBeginFrame(void)
{
    LARGE_INTEGER counter,interval;
    double now,delay;
    int first=!hFrameTimer;
    if(first) {
        hFrameTimer=CreateWaitableTimerExW(0,0,2,TIMER_ALL_ACCESS);
        if(!hFrameTimer) hFrameTimer=CreateWaitableTimerExW(0,0,0,TIMER_ALL_ACCESS);
        if(!hFrameTimer) ExitProcess(1);
        QueryPerformanceFrequency(&counter);hTimerFrequency=(double)counter.QuadPart;
    }
    QueryPerformanceCounter(&counter);now=(double)counter.QuadPart;
    if(first) hClockStart=hClockPrevious=now;
    hFrameTime=(float)((now-hClockStart)/hTimerFrequency);
    hFrameDelta=(float)((now-hClockPrevious)*(1000./hTimerFrequency));
    hClockPrevious=now;
    hFrameDeadline+=hTimerFrequency/120.;
    // Retain the deadline across frames so scheduler overhead does not drift.
    // After a slow frame or minimize, restart without trying to catch up.
    if(hFrameDeadline<now) hFrameDeadline=now+hTimerFrequency/120.;
    delay=(now-hFrameDeadline)*(10000000./hTimerFrequency);
    // x87 conversion keeps this x86 host independent of CRT 64-bit helpers.
    __asm fld delay
    __asm fistp qword ptr interval
    if(!SetWaitableTimer(hFrameTimer,&interval,0,0,0,FALSE)) ExitProcess(1);
}
static void hEndFrame(void)
{
    if(WaitForSingleObject(hFrameTimer,INFINITE)!=WAIT_OBJECT_0) ExitProcess(1);
}
static void hEndControls(void)
{
    hStopLook(1);
    if(hFrameTimer) {CloseHandle(hFrameTimer);hFrameTimer=0;hFrameDeadline=0;}
}
