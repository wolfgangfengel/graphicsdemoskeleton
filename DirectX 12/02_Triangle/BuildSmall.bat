SET PATH=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.10158.0\um\x86;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.10158.0\ucrt\x86;C:\Windows\SysWOW64;C:\Windows\System32
Tools\crinkler14\crinkler.exe /OUT:Intro.exe /HASHTRIES:500 /SUBSYSTEM:WINDOWS /COMPMODE:SLOW /ORDERTRIES:5000 /TRUNCATEFLOATS:16 /HASHSIZE:500 /PRINT:LABELS /PRINT:IMPORTS /ENTRY:winmain /PRINT:LABELS /REPORT:report.html /RANGE:d3d12.dll dxgi.lib kernel32.lib user32.lib d3d12.lib Win32\Release\Window.obj
copy /B /Y Intro.exe Final\Intro.exe

