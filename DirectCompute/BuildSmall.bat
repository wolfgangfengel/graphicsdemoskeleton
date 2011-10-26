SET PATH=C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x86;C:\Program Files\Microsoft SDKs\Windows\v7.1\Lib;C:\Windows\System32;C:\Program Files\PellesC\Lib\Win;
Tools\crinkler13\crinkler.exe /OUT:Intro.exe /HASHTRIES:30 /SUBSYSTEM:WINDOWS /COMPMODE:SLOW /ORDERTRIES:5000 /TRUNCATEFLOATS:16 /PRINT:IMPORTS /ENTRY:winmain /PRINT:LABELS /REPORT:report.html /RANGE:d3d11.dll /RANGE:d3dx11.dll kernel32.lib user32.lib winmm.lib d3d11.lib d3dx11.lib dxgi.lib d3dcompiler.lib user32.lib output\Window.obj
copy /B /Y Intro.exe Final\Intro.exe

