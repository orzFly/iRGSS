#include "getopt.h"
#include "apihook.h"
#include "irgss_rb.h"
#include "orzlist.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#ifdef	__cplusplus
extern "C" {
#endif

static char* argv0;
static int verbose_flag;
static int debug_flag;
static int rgssversion;
static char* eval_flag;
static char* lib_flag;
static int screen_flag;

static apihook hookCreateFileW;
static apihook hookReadFile;
static apihook hookCloseHandle;
static apihook hookGetFileType;
static apihook hookEnumFontFamiliesExW;

#define verbose(...) do if (verbose_flag) { printf("%s: ", argv0); puts(#__VA_ARGS__); } while(0)
#define verbosef(...) do if (verbose_flag) { printf("%s: ", argv0); printf(__VA_ARGS__); } while(0)
#define error(...) do fprintf(stderr, "%s: %s", argv0, #__VA_ARGS__); while(0)
#define oassert(k, ...) do if (!(k)) { error(__VA_ARGS__); exit(1);} while(0)

typedef void   (*RGSSInitialize)(HMODULE hRgssDll);
typedef void   (*RGSSGameMain)(HWND hWnd, const char* pScriptNames, char** pRgssadName);

typedef struct {
	int pos;
	char* content;
	int size;
} hookedfilehandle;

static orzlist* hookedfiles;

int
exists(const char *fname)
{
	FILE *file;
	if (file = fopen(fname, "r"))
	{
		fclose(file);
		return 1;
	}
	return 0;
}

int
findpath(const char * fname, char **out, int maxlen)
{
	char *s = getenv("path"), *str, *p, *buf, *pp;
	int len = strlen(s);
	
	buf = calloc(len + 1, 1);
	memcpy(buf, s, len);
	
	str = buf;
	if (!s) {free(buf);return 0;}
	for(p = strchr(str, ';'); p; p = strchr(str, ';')){
		*p = 0;
		verbosef("findpath: searching %s\n", str);
		
		pp = calloc(strlen(str) + strlen(fname) + 2, 1);
		memcpy(pp, str, strlen(str));
		strcat(strcat(pp, "\\"), fname);
		
		if (exists(pp))
		{
			int ret = strlen(pp);
			strncpy(out, pp, maxlen);
			
			free(pp);
			free(buf);
			
			verbosef("findpath: hit %s\n", out);
			return ret;
		}
		
		free(pp);
		str = p + 1;
	}
	verbose(findpath: not found);
	free(buf);
	
	return 0;
}

HWND
creatergsswindow(const char* classname, const char* title, int sw, int sh, int hide)
{
	int width, height;
	RECT rt;
	WNDCLASSA winclass;
	HWND hWnd;
	DWORD dwStyle;

	winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = DefWindowProc;
	winclass.cbClsExtra   = 0;
	winclass.cbWndExtra   = 0;
	winclass.hInstance   = 0;
	winclass.hIcon    = LoadIcon(0, IDI_APPLICATION);
	winclass.hCursor   = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = classname;

	if (!RegisterClassA(&winclass))
	{
		error(failed to register class);
		exit(1);
	}

	width = sw + GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
	height = sh + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);

	if (hide)
	{
		rt.left   = -GetSystemMetrics(SM_CXSCREEN) * 1000;
		rt.top   = -GetSystemMetrics(SM_CYSCREEN) * 1000;
	}
	else
	{
		rt.left   = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
		rt.top   = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	}
	
	rt.right = rt.left + width;
	rt.bottom = rt.top + height;

	dwStyle = (WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE);

	hWnd = CreateWindowExA(WS_EX_WINDOWEDGE, classname, title, dwStyle,
	   rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, 0, 0, 0, 0);
	if (!hWnd)
	{
		error(failed to create window);
		exit(1);
	}

	return hWnd;
}

HANDLE
	WINAPI
	hookfCreateFileW(
	__in     LPCWSTR lpFileName,
	__in     DWORD dwDesiredAccess,
	__in     DWORD dwShareMode,
	__in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	__in     DWORD dwCreationDisposition,
	__in     DWORD dwFlagsAndAttributes,
	__in_opt HANDLE hTemplateFile
	)
{
	HANDLE ret;

	if (lstrcmp(lpFileName,L":scripts")==0)
	{
		hookedfilehandle* f = calloc(sizeof(hookedfilehandle), 1);
		f->content = irgss_scripts;
		f->size = sizeof(irgss_scripts);
		f->pos = 0;
		
		orzlist_prepend(hookedfiles, (void*)f, (void*)f);
		verbosef("hook hit: CreateFileW: hookedfilehandle: %d\n", (int)f);
		
		return (HANDLE)f;
	}

	apihook_off(hookCreateFileW);
	ret = CreateFileW(lpFileName,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile);
	apihook_on(hookCreateFileW);
	return ret;
}


BOOL WINAPI hookfReadFile(
	HANDLE hFile, 
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
	)
{

	if (orzlist_get(hookedfiles, (void*)hFile) == hFile)
	{
		hookedfilehandle* f = (hookedfilehandle*)hFile;
		int count;
		verbosef("hook hit: ReadFile: hookedfilehandle: %d, pos: %d, size: %d\n", (int)f, f->pos, f->size);
		verbosef("hook hit: ReadFile: nNumberOfBytesToRead: %d\n", nNumberOfBytesToRead);
		count = f->size - f->pos;
		if (count > nNumberOfBytesToRead)
			count = nNumberOfBytesToRead;
		
		if (count == 0)
		{
			verbosef("hook hit: ReadFile: EOF\n", nNumberOfBytesToRead);
			(*lpNumberOfBytesRead)=0;
			return TRUE;
		}
		
		(*lpNumberOfBytesRead)=count;
		if (lpBuffer!=NULL){
			memset(lpBuffer,0,nNumberOfBytesToRead);
			memcpy(lpBuffer,f->content + f->pos,count);
			f->pos += count;
		}
		return count;
	}
	else
	{
		BOOL ret;
		apihook_off(hookReadFile);
		ret=ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
		apihook_on(hookReadFile);
		return ret;
	}
}

BOOL WINAPI hookfCloseHandle(HANDLE h)
{
	if (orzlist_get(hookedfiles, (void*)h) == h)
	{
		verbosef("hook hit: CloseHandle: hookedfilehandle: %d\n", (int)h);
		orzlist_remove(hookedfiles, (void*)h);
		return TRUE;
	}
	else
	{
		BOOL ret;
		apihook_off(hookCloseHandle);
		ret=CloseHandle(h);
		apihook_on(hookCloseHandle);
		return ret;
	}
}

int
	WINAPI
	hookfEnumFontFamiliesExW(
		__in HDC hdc, 
		__in LPLOGFONTW lpLogfont, 
		__in FONTENUMPROCW lpProc, 
		__in LPARAM lParam, 
		__in DWORD dwFlags
	)
{
	int ret;
	LOGFONTW* lpelfe;
	TEXTMETRICW* lpntme;
	
	verbose(hook hit: EnumFontFamiliesExW: enter);
	apihook_off(hookEnumFontFamiliesExW);
	
	lpelfe = calloc(sizeof(LOGFONTW), 1);
	lpntme = calloc(sizeof(TEXTMETRICW), 1);
	
	verbose(hook hit: EnumFontFamiliesExW: fake VL Gothic);
	wcscpy(lpelfe->lfFaceName, L"VL Gothic");
	if (!lpProc(lpelfe, lpntme, TRUETYPE_FONTTYPE, lParam)) goto done;
	
	verbose(hook hit: EnumFontFamiliesExW: fake VL PGothic);
	wcscpy(lpelfe->lfFaceName, L"VL PGothic");
	if (!lpProc(lpelfe, lpntme, TRUETYPE_FONTTYPE, lParam)) goto done;
	
	verbose(hook hit: EnumFontFamiliesExW: back to real world);
	ret = EnumFontFamiliesExW(hdc, lpLogfont, lpProc, lParam, dwFlags);
done:
	free(lpelfe);
	free(lpntme);
	
	apihook_on(hookEnumFontFamiliesExW);
	verbose(hook hit: EnumFontFamiliesExW: leave);
	return ret;
}

DWORD
	WINAPI
	hookfGetFileType(_In_ HANDLE hFile)
{
	DWORD ret;
	if (orzlist_get(hookedfiles, (void*)hFile) == hFile)
	{
		verbosef("hook hit: GetFileType: hookedfilehandle: %d\n", (int)hFile);
		return FILE_TYPE_CHAR;
	}
	
	apihook_off(hookGetFileType);
	ret=GetFileType(hFile);
	apihook_on(hookGetFileType);
	return ret;
}

int
main (int argc, char **argv)
{
	int c;
	argv0 = argv[0];

	while (1)
 	{
		static struct option long_options[] =
		{
			{"verbose", no_argument,       &verbose_flag, 1},
			{"brief",   no_argument,       &verbose_flag, 0},
			
			{"debug",   no_argument,       0, 'd'},
			{"screen",  no_argument,       0, 's'},
			{"eval",    required_argument, 0, 'e'},
			//{"delete",  required_argument, 0, 'd'},
			//{"create",  required_argument, 0, 'c'},
			{0, 0, 0, 0}
		};

		int option_index = 0;
		
		c = getopt_long (argc, argv, "sde:", long_options, &option_index);
		
		if (c == -1)
			break;
		
		switch (c)
		{
			case 0:
				break;

			case 'd':
				debug_flag = 1;
				break;

			case 'e':
				eval_flag = optarg;
				break;

			case '?':
				/* getopt_long already printed an error message. */
				break;

			case 's':
				screen_flag = 1;
				break;
				
			default:
				abort();
		}
	}

	if (optind < argc)
	{
		while (optind < argc)
		{
			lib_flag = argv[optind++];
		}
	}
	
	if (!lib_flag)
		lib_flag = "RGSS300.dll";
	
	verbosef("lib = %s\n", lib_flag);
	
	if( exists( lib_flag ) ) {
		verbose(lib is exist);
	}
	else
	{
		char path[MAX_PATH + 1];
		verbose(cannot find it here);
		verbose(searching in PATH);
		
		if (findpath(lib_flag, &path, MAX_PATH))
		{
			lib_flag = path;
			verbosef("lib = %s\n", lib_flag);
			if( exists( lib_flag ) ) {
				verbose(lib is exist);
			}
			else
			{
				lib_flag = 0;
			}
		}
		else
		{
			lib_flag = 0;
		}
		
		if (!lib_flag)
		{
			error(lib not found);
			exit(1);
		}
	}
	
	{
		HMODULE hrgss = LoadLibraryA(lib_flag);
		RGSSInitialize frgssInitialize = NULL;
		RGSSGameMain frgssGameMain = NULL;
		HWND hWnd;
		
		oassert(hrgss, failed to initialize lib);
		verbose(lib initialized);
		
		verbose(looking for RGSSInitialize);
		frgssInitialize = (RGSSInitialize) GetProcAddress(hrgss, "RGSSInitialize");
		if (frgssInitialize)
		{
			rgssversion = 1;
			verbose(found RGSS 1);
			goto foundrgss;
		}
		frgssInitialize = (RGSSInitialize) GetProcAddress(hrgss, "RGSSInitialize2");
		if (frgssInitialize)
		{
			rgssversion = 2;
			verbose(found RGSS 2);
			goto foundrgss;
		}
		frgssInitialize = (RGSSInitialize) GetProcAddress(hrgss, "RGSSInitialize3");
		if (frgssInitialize)
		{
			rgssversion = 3;
			verbose(found RGSS 3);
			goto foundrgss;
		}
		oassert(frgssInitialize, failed to find RGSSInitialize);
foundrgss:
		verbose(looking for RGSSGameMain);
		frgssGameMain = (RGSSGameMain) GetProcAddress(hrgss, "RGSSGameMain");
		oassert(frgssGameMain, failed to find RGSSGameMain);
		
		verbose(calling RGSSInitialize);
		frgssInitialize(hrgss);
		
		verbose(creating game window for RGSS);
		hWnd = creatergsswindow("iRGSS", "iRGSS", 640, 480, screen_flag);
		
		verbose(setting hook);
		hookedfiles = orzlist_create();
		#define sethook(lib, proc) do { \
			                           hook##proc = apihook_initialize(lib,#proc,(FARPROC)hookf##proc); \
			                           apihook_on(hook##proc); \
                                      } while(0)
        sethook(L"kernel32.dll", CreateFileW);
        sethook(L"kernel32.dll", ReadFile);
        sethook(L"kernel32.dll", CloseHandle);
        sethook(L"kernel32.dll", GetFileType);
        if (rgssversion == 3)
			sethook(L"gdi32.dll", EnumFontFamiliesExW);
		
		verbose(calling RGSSGameMain);
		if (rgssversion < 2)
			frgssGameMain(hWnd, ":scripts", "\0\0\0\0");
		else
			frgssGameMain(hWnd, ":\0s\0c\0r\0i\0p\0t\0s\0\0", "\0\0\0\0");
		
		// open "scripts", "wb" do |io| io.write Marshal.dump [[0,"iRGSS Initialize", Zlib::Deflate.deflate("eval((open('love.txt', 'rb') do |i| i.read end), binding, 'love.txt')")]] end
		//ShowWindow(hWnd, SW_HIDE);
	}
	//{char* j;gets(&j);}
	
	exit (0);
}




#ifdef	__cplusplus
}
#endif