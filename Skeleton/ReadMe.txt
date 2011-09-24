Notes about how to compile:

You need the DirecX June 2010 SDK installed.

Pelles C:
Includes & Lib
- Add the include and lib path for the DirectX SDK
- Add the include file to the Windows 7.1 SDK. In my case 
C:\Program Files\Microsoft SDKs\Windows\v7.1\Include


There will be an error -coming from a v7.1 file- with a return type: just typecast (wchar *) 