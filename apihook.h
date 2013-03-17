#pragma once
#include <Windows.h>
#ifdef	__cplusplus
extern "C" {
#endif

typedef void* apihook;

apihook apihook_initialize(LPCWSTR lpLibFileName , LPCSTR lpProcName , FARPROC lpNewFunc);
void apihook_unlock(apihook self);
void apihook_lock(apihook self);
void apihook_on (apihook self);
void apihook_off (apihook self);
void apihook_dispose(apihook self);

#ifdef	__cplusplus
}
#endif