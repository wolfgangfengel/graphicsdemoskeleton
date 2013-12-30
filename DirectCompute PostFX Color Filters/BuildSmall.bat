SET PATH=C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x86;C:\Program Files (x86)\Windows Kits\8.1\bin\x86;C:\Program Files (x86)\Windows Kits\8.0\Lib\win8\um\x86;C:\Windows\System32
Tools\crinkler14\crinkler.exe /OUT:Intro.exe /HASHTRIES:30 /SUBSYSTEM:WINDOWS /COMPMODE:SLOW /ORDERTRIES:5000 /TRUNCATEFLOATS:16 /PRINT:IMPORTS /ENTRY:winmain /PRINT:LABELS /REPORT:report.html /RANGE:d3d11.dll kernel32.lib user32.lib winmm.lib d3d11.lib d3dcompiler.lib Release\Window.obj
copy /B /Y Intro.exe Final\Intro.exe

