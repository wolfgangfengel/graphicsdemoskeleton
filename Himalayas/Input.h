/* Shared free-fly camera. Coordinates use the shader's ten-metre scene units. */
#define WINWIDTH 1280
#define WINHEIGHT 720
static float hConstants[12]={WINWIDTH,WINHEIGHT,0,0,397.5f,73,-273,-.09966865f,.177226f,0,0,0};
static UINT hFrameNumber;
static HWND hLookWindow;
static POINT hSavedCursor;
static float hSin(float x) { __asm fld x __asm fsin }
static float hCos(float x) { __asm fld x __asm fcos }
static float hSqrt(float x) { __asm fld x __asm fsqrt }
static void hStopLook(int restore)
{
    if(hLookWindow) {
        HWND window=hLookWindow;hLookWindow=0;
        if(GetCapture()==window) ReleaseCapture();
        ShowCursor(TRUE);
        if(restore && GetForegroundWindow()==window) SetCursorPos(hSavedCursor.x,hSavedCursor.y);
    }
}
static POINT hLookCenter(HWND window)
{
    RECT rect;POINT point;
    GetClientRect(window,&rect);point.x=rect.right/2;point.y=rect.bottom/2;
    ClientToScreen(window,&point);return point;
}
static void hResetCamera(void)
{
    hConstants[4]=397.5f;hConstants[5]=73;hConstants[6]=-273;
    hConstants[7]=-.09966865f;hConstants[8]=.177226f;++hConstants[9];
}
static void hRotateCamera(float x,float y)
{
    hConstants[7]-=x*.002f;hConstants[8]-=y*.002f;
    if(hConstants[8]>1.5f) hConstants[8]=1.5f;
    if(hConstants[8]<-1.5f) hConstants[8]=-1.5f;
    if(hConstants[7]>3.14159265f) hConstants[7]-=6.2831853f;
    if(hConstants[7]<-3.14159265f) hConstants[7]+=6.2831853f;
}
static void hMoveCamera(float forward,float right,float up,float distance)
{
    float sy=hSin(hConstants[7]),cy=hCos(hConstants[7]),cp=hCos(hConstants[8]);
    float x=forward*sy*cp-right*cy,y=forward*hSin(hConstants[8])+up,z=forward*cy*cp+right*sy;
    float length=hSqrt(x*x+y*y+z*z);
    if(length>0) {
        distance/=length;
        hConstants[4]+=x*distance;hConstants[5]+=y*distance;hConstants[6]+=z*distance;
    }
}
static int hKey(int key) {return (GetAsyncKeyState(key)&0x8000)!=0;}
static void hUpdateCamera(HWND window,float delta)
{
    if(GetForegroundWindow()!=window) {hStopLook(0);return;}
    if(hLookWindow && GetCapture()!=hLookWindow) hStopLook(0);
    if(hLookWindow) {
        POINT cursor,center=hLookCenter(window);
        GetCursorPos(&cursor);hRotateCamera((float)(cursor.x-center.x),(float)(cursor.y-center.y));
        SetCursorPos(center.x,center.y);
    }
    if(delta>100) delta=100; // No jump after minimizing or a slow frame.
    hMoveCamera((float)(hKey('W')-hKey('S')),(float)(hKey('D')-hKey('A')),
        (float)(hKey('E')-hKey('Q')),(float)delta*.01f*(hKey(VK_SHIFT)?4:1));
}
/* Return true for handled messages so the stock edit window has no text/menu UI. */
static int hInput(const MSG *message)
{
    switch(message->message) {
    case WM_RBUTTONDOWN:
        if(!hLookWindow) {
            POINT center=hLookCenter(message->hwnd);
            GetCursorPos(&hSavedCursor);hLookWindow=message->hwnd;
            SetCapture(hLookWindow);ShowCursor(FALSE);SetCursorPos(center.x,center.y);
        }
        return 1;
    case WM_RBUTTONUP: hStopLook(1);return 1;
    case WM_KILLFOCUS: case WM_CANCELMODE: case WM_CAPTURECHANGED: hStopLook(0);return 1;
    case WM_KEYDOWN:
        if(message->wParam=='R' && !(message->lParam&(1L<<30))) hResetCamera();
        return 1;
    case WM_CHAR: case WM_CONTEXTMENU: case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN: case WM_LBUTTONUP: return 1;
    }
    return 0;
}
#include "Frame.h"
/* A/B are fixed-size lookup images. C and D each have two history images. */
static void hPassImages(UINT pass,UINT parity,UINT *output,int inputs[4])
{
    UINT oldC=2+parity,newC=2+(parity^1),oldD=4+parity,newD=4+(parity^1),n;
    for(n=0;n<4;++n) inputs[n]=-1;
    *output=pass;
    if(pass==2) { *output=newC; inputs[0]=oldC; inputs[1]=oldD; }
    if(pass==3) { *output=newD; inputs[0]=0; inputs[1]=oldD; inputs[2]=newC; inputs[3]=1; }
    if(pass==4) { *output=6; inputs[0]=newC; inputs[1]=newD; }
}
