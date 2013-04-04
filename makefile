nulfile=nul
nul=>$(nulfile) 2>$(nulfile)
cc=cl
link=link
ruby=ruby
rm=del
rmpost=$(nul)||echo.$(nul)

cflags_common=/nologo /W3 /EHsc /D "WIN32" /D "UNICODE" /D "_CONSOLE" /D "_CRT_SECURE_NO_WARNINGS" /FD /c
lflags_common=apihook.lib kernel32.lib shell32.lib user32.lib gdi32.lib /nologo /subsystem:console /machine:I386 /out:irgss.exe

!if "$(mode)" == "release"
cflags=$(cflags_common) /D "NDEBUG" /O2 /Ox
lflags=$(lflags_common) /incremental:no
!else
mode=debug
cflags=$(cflags_common) /Zi /O2 /D _DEBUG
lflags=$(lflags_common) /incremental:no /DEBUG /OPT:REF /OPT:ICF
!endif

.cpp.obj:
    $(cc) $(cflags) $<

.c.obj:
    $(cc) $(cflags) $<

.obj.lib:
    $(link) /lib /NOLOGO $** /out:$@

all: irgss.exe

irgss.obj: irgss_rb.h

irgss_rb.h: irgss.rb mkirgss_rb.rb
    $(ruby) mkirgss_rb.rb

apihook.lib: apihook.obj

irgss.exe: getopt.obj irgss.obj orzlist.obj orzstr.obj apihook.lib
    $(link) getopt.obj orzlist.obj orzstr.obj irgss.obj $(lflags)

clean:
    $(rm) apihook.obj apihook.lib getopt.obj orzstr.obj orzlist.obj irgss_rb.h irgss.obj irgss.exe irgss.ilk irgss.pdb vc90.idb vc90.pdb $(rmpost)

rebuild: clean all