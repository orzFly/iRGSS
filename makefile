
cc=cl
link=link
ruby=ruby

cflags=/nologo /W3 /EHsc /O2 /D "NDEBUG" /D "WIN32" /D "UNICODE" /D "_CONSOLE" /FD /c 
lflags=apihook.lib kernel32.lib shell32.lib user32.lib gdi32.lib /nologo /subsystem:console /incremental:yes /machine:I386 /out:irgss.exe

all: irgss.exe

apihook.obj: apihook.cpp
    $(cc) $(cflags) apihook.cpp

apihook.lib: apihook.obj
    $(link) /lib apihook.obj /out:apihook.lib

getopt.obj: getopt.c
    $(cc) $(cflags) getopt.c

orzlist.obj: orzlist.c
    $(cc) $(cflags) orzlist.c

irgss.obj: irgss_rb.h irgss.c
    $(cc) $(cflags) irgss.c

irgss_rb.h: irgss.rb mkirgss_rb.rb
    $(ruby) mkirgss_rb.rb

irgss.exe: getopt.obj irgss.obj orzlist.obj apihook.lib
    $(link) getopt.obj orzlist.obj irgss.obj $(lflags)
