#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
static HWND focus;
static SHORT keys[256];
static HWND WINAPI testForeground(void) {return focus;}
static SHORT WINAPI testKeys(int key) {return keys[key];}
#define GetForegroundWindow testForeground
#define GetAsyncKeyState testKeys
#include "../../../Himalayas/Input.h"
static void checkNear(float a,float b) {assert(fabs(a-b)<.001);}
int __cdecl main(void)
{
    MSG message={0};float epoch;
    HWND window=CreateWindowW(L"edit",L"input test",0,0,0,1,1,0,0,0,0);
    hConstants[4]=hConstants[5]=hConstants[6]=hConstants[7]=hConstants[8]=0;
    focus=window;keys['W']=-32768;
    hUpdateCamera(window,8.333333f);checkNear(hConstants[6],.08333333f);
    keys[VK_SHIFT]=-32768;hUpdateCamera(window,8.333333f);checkNear(hConstants[6],.41666667f);
    focus=0;hUpdateCamera(window,100);checkNear(hConstants[6],.41666667f);
    keys['W']=keys[VK_SHIFT]=0;focus=window;keys['E']=-32768;
    hUpdateCamera(window,1000);checkNear(hConstants[5],1); // Pause capped to 100 ms.
    hConstants[4]=hConstants[5]=hConstants[6]=0;
    hMoveCamera(1,1,1,10);checkNear(hSqrt(hConstants[4]*hConstants[4]+hConstants[5]*hConstants[5]+hConstants[6]*hConstants[6]),10);
    hRotateCamera(0,-100000);checkNear(hConstants[8],1.5f);
    hRotateCamera(0,100000);checkNear(hConstants[8],-1.5f);
    epoch=hConstants[9];message.message=WM_KEYDOWN;message.wParam='R';
    hInput(&message);checkNear(hConstants[4],397.5f);checkNear(hConstants[5],73);checkNear(hConstants[6],-273);checkNear(hConstants[9],epoch+1);
    message.lParam=1L<<30;hInput(&message);checkNear(hConstants[9],epoch+1);
    DestroyWindow(window);puts("Camera speed, boost, focus, pause clamp, diagonal, pitch and reset checks passed");return 0;
}
