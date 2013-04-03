
nulfile=nul
nul=>$(nulfile) 2>$(nulfile)
cc=cl
link=link
ruby=ruby
rm=del
rmpost=$(nul)||echo.$(nul)

release_cflags=/nologo /W3 /EHsc /O2 /Ox /D "NDEBUG" /D "WIN32" /D "UNICODE" /D "_CONSOLE" /FD /c 
release_lflags=apihook.lib kernel32.lib shell32.lib user32.lib gdi32.lib /nologo /subsystem:console /incremental:yes /machine:I386 /out:irgss.exe

cflags=/nologo /W3 /EHsc /Zi /O2 /D _DEBUG /D "WIN32" /D "UNICODE" /D "_CONSOLE" /FD /c 
lflags=apihook.lib kernel32.lib shell32.lib user32.lib gdi32.lib /nologo /subsystem:console /incremental:yes /machine:I386 /out:irgss.exe /DEBUG /OPT:REF /OPT:ICF

all: irgss.exe

apihook.obj: apihook.cpp
    $(cc) $(cflags) apihook.cpp

apihook.lib: apihook.obj
    $(link) /lib apihook.obj /out:apihook.lib

getopt.obj: getopt.c
    $(cc) $(cflags) getopt.c

orzlist.obj: orzlist.c
    $(cc) $(cflags) orzlist.c

orzstr.obj: orzstr.c
    $(cc) $(cflags) orzstr.c

irgss.obj: irgss_rb.h irgss.c
    $(cc) $(cflags) irgss.c

irgss_rb.h: irgss.rb mkirgss_rb.rb
    $(ruby) mkirgss_rb.rb

irgss.exe: getopt.obj irgss.obj orzlist.obj orzstr.obj apihook.lib
    $(link) getopt.obj orzlist.obj orzstr.obj irgss.obj $(lflags)

clean:
    $(rm) apihook.obj $(rmpost)
    $(rm) apihook.lib $(rmpost)
    $(rm) getopt.obj $(rmpost)
    $(rm) orzstr.obj $(rmpost)
    $(rm) orzlist.obj $(rmpost)
    $(rm) irgss_rb.h $(rmpost)
    $(rm) irgss.obj $(rmpost)
    $(rm) irgss.exe $(rmpost)
    $(rm) vc90.idb $(rmpost)
    $(rm) irgss.ilk $(rmpost)
    $(rm) irgss.pdb $(rmpost)
    $(rm) vc90.pdb $(rmpost)

rebuild: clean all