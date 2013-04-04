#include "getopt.h"
#include "apihook.h"
#include "irgss_rb.h"
#include "orzlist.h"
#include "orzstr.h"
#include <locale.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#ifdef	__cplusplus
extern "C" {
#endif

static char* argv0;
static int rgssversion;
static int rgssinit;
static char* libname;
static orzsb* code_settings;

static int flag_verbose;
static int flag_debug;
static int flag_screen;
static int flag_rc;
static char* flag_rcname;

static apihook hookCreateFileW;
static apihook hookReadFile;
static apihook hookCloseHandle;
static apihook hookGetFileType;
static apihook hookSetWindowPos;
static apihook hookShowWindow;
static apihook hookMessageBoxW;
static apihook hookSetForegroundWindow;
static apihook hookEnumFontFamiliesExW;

#define verbose(...) do if (flag_verbose) { printf("%s: ", argv0); puts(#__VA_ARGS__); } while(0)
#define verbosef(...) do if (flag_verbose) { printf("%s: ", argv0); printf(__VA_ARGS__); } while(0)
#define error(...) do fprintf(stderr, "%s: %s\n", argv0, #__VA_ARGS__); while(0)
#define oassert(k, ...) do if (!(k)) { error(__VA_ARGS__); exit(1);} while(0)
#define hookon(proc) do { \
	                     verbose(hook: enabling: ##proc); \
                         apihook_on(hook##proc); \
                        } while(0)
#define hookoff(proc) do { \
	                      verbose(hook: disabling: ##proc); \
                          apihook_off(hook##proc); \
                         } while(0)
#define hookons(proc) do { \
                         apihook_on(hook##proc); \
                        } while(0)
#define hookoffs(proc) do { \
                          apihook_off(hook##proc); \
                         } while(0)
typedef void   (*RGSSInitialize)(HMODULE hRgssDll);
typedef void   (*RGSSGameMain)(HWND hWnd, const char* pScriptNames, char* pRgssadName);
typedef int    (*RGSSEval)(const char* pScripts);

static HMODULE hrgss = NULL;
static RGSSInitialize frgssInitialize = NULL;
static RGSSGameMain frgssGameMain = NULL;
static RGSSEval frgssEval = NULL;

typedef struct {
	char* filename;
	int pos;
	const char* content;
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

char*
findpath(const char * fname)
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
			free(buf);
			
			verbosef("findpath: hit %s\n", pp);
			return pp;
		}
		
		free(pp);
		str = p + 1;
	}
	verbose(findpath: not found);
	free(buf);
	
	return 0;
}

LRESULT
WINAPI rgsswindowwndproc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if (Msg == WM_ACTIVATEAPP)
	{
		return 0;
	}
	
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

HWND
creatergsswindow(const wchar_t* classname, const wchar_t* title, int sw, int sh, int hide)
{
	int width, height;
	RECT rt;
	WNDCLASSW winclass;
	HWND hWnd;
	DWORD dwStyle;

	winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = rgsswindowwndproc;
	winclass.cbClsExtra   = 0;
	winclass.cbWndExtra   = 0;
	winclass.hInstance   = 0;
	winclass.hIcon    = LoadIcon(0, IDI_APPLICATION);
	winclass.hCursor   = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = classname;

	if (!RegisterClassW(&winclass))
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

	dwStyle = (WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

	hWnd = CreateWindowExW(WS_EX_WINDOWEDGE, classname, title, dwStyle,
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
		char* code;
		hookedfilehandle* f = calloc(sizeof(hookedfilehandle), 1);
		f->filename = ":scripts";
		f->content = irgss_scripts;
		f->size = sizeof(irgss_scripts);
		f->pos = 0;
		
		if (rgssinit == FALSE)
		{
			verbose(hook hit: CreateFileW: loading RGSS scripts...);
			verbose(stopping windows hooks...);
			hookoff(SetWindowPos);
			hookoff(ShowWindow);
			hookoff(SetForegroundWindow);
			if (rgssversion == 3)
				hookoff(EnumFontFamiliesExW);
			
			verbose(loading iRGSS header scripts...);
			frgssEval(irgss_header);
			
			verbose(loading iRGSS settings...);
			
			code = orzsb_build(code_settings);
			if (flag_verbose){
				puts("```ruby");
				puts(code);
				puts("```");
			}
			frgssEval(code);
			
			free(code);
			orzsb_clean(code_settings);
			free(code_settings);
			
			rgssinit = 233;
		}
		
		orzlist_prepend(hookedfiles, (void*)f, (void*)f);
		verbosef("hook hit: CreateFileW: hookedfilehandle: %d, filename: %s\n", (int)f, f->filename);
		
		return (HANDLE)f;
	}

	hookoffs(CreateFileW);
	ret = CreateFileW(lpFileName,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile);
	hookons(CreateFileW);
	return ret;
}

BOOL
WINAPI
hookfSetWindowPos(
    __in HWND hWnd,
    __in_opt HWND hWndInsertAfter,
    __in int X,
    __in int Y,
    __in int cx,
    __in int cy,
    __in UINT uFlags)
{
	verbose(hook hit: SetWindowPos: ignoring...);
	return 0;
}

BOOL
WINAPI
hookfSetForegroundWindow(
    __in HWND hWnd)
{
	verbose(hook hit: SetForegroundWindow: ignoring);
	return 0;
}

BOOL
WINAPI
hookfShowWindow(
    __in HWND hWnd,
    __in int nCmdShow)
{
	verbose(hook hit: ShowWindow: ignoring);
	return 0;
}

int
WINAPI
hookfMessageBoxW(
    __in_opt HWND hWnd,
    __in_opt LPCWSTR lpText,
    __in_opt LPCWSTR lpCaption,
    __in UINT uType)
{
	int choice = 1;
	if ((uType & 0xF) == 0) {
		verbose(hook hit: MessageBoxW: so simple, so redirect to STDOUT);
		wprintf(L"[%ls]\n%ls\n", lpCaption, lpText);
		choice = 1;
	}
	else
	{
		hookoffs(MessageBoxW);
		choice = MessageBoxW(0, lpText, lpCaption, uType);
		hookons(MessageBoxW);
	}
	
	return choice;
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
		unsigned int count;
		verbosef("hook hit: ReadFile: hookedfilehandle: %d, filename: %s, pos: %d, size: %d\n", (int)f, f->filename, f->pos, f->size);
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
		hookoffs(ReadFile);
		ret=ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
		hookons(ReadFile);
		return ret;
	}\
}

BOOL WINAPI hookfCloseHandle(HANDLE h)
{
	if (orzlist_get(hookedfiles, (void*)h) == h)
	{
		hookedfilehandle* f = (hookedfilehandle*)h;
		verbosef("hook hit: CloseHandle: hookedfilehandle: %d, filename: %s\n", (int)f, f->filename);
		orzlist_remove(hookedfiles, (void*)h);
		
		if (f->content == irgss_scripts && rgssinit == 233)
		{
			rgssinit = TRUE;
			verbose(hook hit: CloseHandle: RGSS scripts loaded);
			verbose(stopping file hooks);
			hookoff(CreateFileW);
	        hookoff(ReadFile);
	        hookoff(CloseHandle);
	        hookoff(GetFileType);
		}
		return TRUE;
	}
	else
	{
		BOOL ret;
		hookoffs(CloseHandle);
		ret=CloseHandle(h);
		hookons(CloseHandle);
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
	hookoffs(EnumFontFamiliesExW);
	
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
	
	hookons(EnumFontFamiliesExW);
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
	
	hookoffs(GetFileType);
	ret=GetFileType(hFile);
	hookons(GetFileType);
	return ret;
}

static const char* short_options = "abir:e:f:sdmc:C";
static struct option long_options[] =
{
	{"verbose", no_argument,       &flag_verbose, 1},
	{"brief",   no_argument,       &flag_verbose, 0},
	
	{"require",			required_argument, 	0, 'r'},
	{"eval",			required_argument, 	0, 'e'},
	{"file",			required_argument, 	0, 'f'},
	
	{"inspect",			no_argument,		0, 'i'},
	{"benchmark",		no_argument,		0, 'b'},
	{"batch",			no_argument,		0, 'a'},
	
	{"irb-math",		no_argument, 		0, 'm'},
	{"irb-inspect",		no_argument, 		0, 2001},
	{"irb-noinspect",	no_argument, 		0, 2002},
	{"irb-name",		required_argument, 	0, 2003},
	{"irb-echo",		no_argument, 		0, 2004},
	{"irb-noecho",		no_argument, 		0, 2005},
	{"irb-norc",		no_argument, 		0, 2006},
	{"irb-prompt-mode",	required_argument, 	0, 2007},
	{"irb-prompt",		required_argument, 	0, 2007},
	{"irb-noprompt",	no_argument, 		0, 2008},
	{"irb-inf-ruby-mode",no_argument, 		0, 2009},
	{"irb-sample-book-mode",no_argument, 	0, 2010},
	{"irb-simple-prompt",no_argument, 		0, 2010},
	{"irb-tracer",		no_argument, 		0, 2011},
	{"irb-back-trace-limit",required_argument, 0, 2012},
	{"irb-context-mode",required_argument, 	0, 2013},
	{"irb-single",		no_argument, 		0, 2014},
	{"irb-debug",		required_argument, 	0, 2015},
	
	{"logo",  			no_argument,       	0, 9998},
	{"no-logo",  		no_argument,       	0, 9999},
	{"debug", 	 		no_argument,       	0, 'd'},
	{"screen",		  	no_argument,       	0, 's'},
	
	{"rc",				required_argument, 	0, 'c'},
	{"no-rc",			no_argument, 		0, 'C'},
	
	{0, 0, 0, 0}
};

#define codesettings(x) do {orzsb_puts(code_settings, x);} while(0)
#define codesettingsoptarg(x) do {\
								char* escaped = addslash(optarg);\
								orzsb_printf(code_settings, x, escaped);\
								orzsb_printf(code_settings, "\n");\
								free(escaped);\
								break;\
							  } while(0)
void
parseoptions(int argc, char **argv)
{
	int c;
	
	while (1)
 	{
		int option_index = 0;
		
		c = getopt_long (argc, argv, short_options, long_options, &option_index);
		
		if (c == -1)
			break;
		
		switch (c)
		{
			case 'C':
			case 'c':
			case 0:
				break;

			case 'd':
				flag_debug = 1;
				codesettings("$DEBUG = true");
				if (rgssversion >= 2)
					codesettings("$TEST = true");
				break;

			case 's':
				flag_screen = 1;
				break;
			
			case 'e':	codesettingsoptarg("::IRGSS::VAR[:file]=nil; (::IRGSS::VAR[:eval]||=[]) << '%s'"); break;
			case 'f':	codesettingsoptarg("::IRGSS::VAR[:eval]=nil; ::IRGSS::VAR[:file] = '%s'"); break;
			case 'r':	codesettingsoptarg("(::IRGSS::VAR[:required]||=[]) << '%s'"); break;
			
			case 'a':	/* todo */ break;
			case 'b':	codesettings("::IRGSS::VAR[:mode] = :benchmark"); break;
			case 'i':	codesettings("::IRGSS::VAR[:mode] = :inspect"); break;
			
			case 'm':	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:MATH_MODE] = true"); break;
			case 2001:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:INSPECT_MODE] = true"); break;
			case 2002:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:INSPECT_MODE] = false"); break;
			case 2003:	codesettingsoptarg("::IRGSS::VAR[:irb_name] = '%s'"); break;
			case 2004:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:ECHO] = true"); break;
			case 2005:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:ECHO] = false"); break;
			case 2006:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:RC] = false"); break;
			case 2007:	codesettingsoptarg("(::IRGSS::VAR[:irb_conf]||={})[:PROMPT_MODE] = '%s'.upcase.tr(\"-\", \"_\").intern"); break;
			case 2008:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:PROMPT_MODE] = :NULL"); break;
			case 2009:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:PROMPT_MODE] = :INF_RUBY"); break;
			case 2010:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:PROMPT_MODE] = :SIMPLE"); break;
			case 2011:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:USE_TRACER] = true"); break;
			case 2012:	codesettingsoptarg("(::IRGSS::VAR[:irb_conf]||={})[:BACK_TRACE_LIMIT] = ('%s'.to_i rescue 0)"); break;
			case 2013:	codesettingsoptarg("(::IRGSS::VAR[:irb_conf]||={})[:CONTEXT_MODE] = ('%s'.to_i rescue 0)"); break;
			case 2014:	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:SINGLE_IRB] = true"); break;
			case 2015:	codesettingsoptarg("(::IRGSS::VAR[:irb_conf]||={})[:DEBUG_LEVEL] = ('%s'.to_i rescue 0)"); break;
			
			case 9998:	codesettings("::IRGSS::VAR[:no_logo] = nil; ::IRGSS::VAR[:logo] = true"); break;
			case 9999:	codesettings("::IRGSS::VAR[:no_logo] = true; ::IRGSS::VAR[:logo] = nil"); break;
			
			default:
				abort();
		}
	}
}

void
runrgss(char* libname)
{
	HWND hWnd;
	
	hrgss = LoadLibraryA(libname);
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
	{
		char* escaped = addslash(libname);
		orzsb_printf(code_settings, "::IRGSS::LIBNAME = '%s'\n", escaped);
		free(escaped);
	}
	orzsb_printf(code_settings, "::IRGSS::PLATFORM = 'RGSS%d'\n", rgssversion);
	codesettings("(::IRGSS::VAR[:irb_conf]||={})[:IRB_NAME] = ::IRGSS::IRBNAME = ::IRGSS::VAR[:irb_name] || File.basename(::IRGSS::LIBNAME, '.dll')");

	verbose(looking for RGSSGameMain);
	frgssGameMain = (RGSSGameMain) GetProcAddress(hrgss, "RGSSGameMain");
	oassert(frgssGameMain, failed to find RGSSGameMain);
	
	verbose(looking for RGSSEval);
	frgssEval = (RGSSEval) GetProcAddress(hrgss, "RGSSEval");
	oassert(frgssEval, failed to find RGSSEval);
	
	verbose(calling RGSSInitialize);
	frgssInitialize(hrgss);
	
	verbose(creating game window for RGSS);
	switch (rgssversion)
	{
		case 1:
			hWnd = creatergsswindow(L"RGSS Player", L"iRGSS1", 640, 480, flag_screen);
			break;
		case 2:
			hWnd = creatergsswindow(L"RGSS2 Player", L"iRGSS2", 544, 416, flag_screen);
			break;
		case 3:
			hWnd = creatergsswindow(L"RGSS3 Player", L"iRGSS3", 544, 416, flag_screen);
			break;
		
	}
	orzsb_printf(code_settings, "::IRGSS::SCREENHWND = %d\n", hWnd);
	
	verbose(setting hook);
	hookedfiles = orzlist_create();
	#define sethook(lib, proc) do { \
		                           hook##proc = apihook_initialize(lib,#proc,(FARPROC)hookf##proc); \
		                           hookon(##proc); \
                                  } while(0)
    sethook(L"kernel32.dll", CreateFileW);
    sethook(L"kernel32.dll", ReadFile);
    sethook(L"kernel32.dll", CloseHandle);
    sethook(L"kernel32.dll", GetFileType);
    sethook(L"user32.dll", ShowWindow);
    sethook(L"user32.dll", SetWindowPos);
    sethook(L"user32.dll", SetForegroundWindow);
    sethook(L"user32.dll", MessageBoxW);
    if (rgssversion == 3)
		sethook(L"gdi32.dll", EnumFontFamiliesExW);
	
	verbose(calling RGSSGameMain);
	if (rgssversion < 2)
		frgssGameMain(hWnd, ":scripts", "\0\0\0\0");
	else
		frgssGameMain(hWnd, ":\0s\0c\0r\0i\0p\0t\0s\0\0", "\0\0\0\0");
}

char*
trylib (char* name)
{
	char* ret;
	verbosef("lib: trying %s\n", name);
	
	if( exists( name ) ) {
		ret = calloc(1, strlen(name) + 1);
		strcpy(ret, name);
		verbose(lib: ready);
		return ret;
	}
	else
	{
		char* path;
		verbose(lib: cannot find it here, so we are going to search it in $PATH);
		
		if (path = findpath(name))
		{
			verbosef("lib: found %s\n", path);
			if( exists( path ) ) {
				return path;
			}
			else
			{
				verbose(lib: failed);
				return 0;
			}
		}
		else
		{
			verbose(lib: failed);
			return 0;
		}
	}
}

void
decidelib(int argc, char **argv)
{
	#define trytrylib(x) do { if (!libname) libname = trylib(x); } while(0)
	libname = 0;
	if (optind < argc)
	{
		while (optind < argc)
		{
			char* arg = argv[optind++];
			if (strcmp(arg, ":rgss3") == 0)
			{
				verbose(searching for some famous RGSS3 library...);
				trytrylib("RGSS301.dll");
				trytrylib("RGSS300.dll");
			}
			else if (strcmp(arg, ":rgss2") == 0)
			{
				verbose(searching for some famous RGSS2 library...);
				trytrylib("RGSS202E.dll");
				trytrylib("RGSS202C.dll");
				trytrylib("RGSS202J.dll");
				trytrylib("RGSS201E.dll");
				trytrylib("RGSS201C.dll");
				trytrylib("RGSS201J.dll");
				trytrylib("RGSS200E.dll");
				trytrylib("RGSS200C.dll");
				trytrylib("RGSS200J.dll");
			}
			else if (strcmp(arg, ":rgss1") == 0)
			{
				verbose(searching for some famous RGSS1 library...);
				trytrylib("RGSS104E.dll");
				trytrylib("RGSS104C.dll");
				trytrylib("RGSS104J.dll");
				trytrylib("RGSS103E.dll");
				trytrylib("RGSS103C.dll");
				trytrylib("RGSS103J.dll");
				trytrylib("RGSS102E.dll");
				trytrylib("RGSS102C.dll");
				trytrylib("RGSS102J.dll");
				trytrylib("RGSS101E.dll");
				trytrylib("RGSS101C.dll");
				trytrylib("RGSS101J.dll");
				trytrylib("RGSS100E.dll");
				trytrylib("RGSS100C.dll");
				trytrylib("RGSS100J.dll");
			}
			else if (strcmp(arg, ":rgss") == 0)
			{
				verbose(searching for some famous RGSS library...);
				trytrylib("RGSS301.dll");
				trytrylib("RGSS300.dll");
				trytrylib("RGSS202E.dll");
				trytrylib("RGSS202C.dll");
				trytrylib("RGSS202J.dll");
				trytrylib("RGSS201E.dll");
				trytrylib("RGSS201C.dll");
				trytrylib("RGSS201J.dll");
				trytrylib("RGSS200E.dll");
				trytrylib("RGSS200C.dll");
				trytrylib("RGSS200J.dll");
				trytrylib("RGSS104E.dll");
				trytrylib("RGSS104C.dll");
				trytrylib("RGSS104J.dll");
				trytrylib("RGSS103E.dll");
				trytrylib("RGSS103C.dll");
				trytrylib("RGSS103J.dll");
				trytrylib("RGSS102E.dll");
				trytrylib("RGSS102C.dll");
				trytrylib("RGSS102J.dll");
				trytrylib("RGSS101E.dll");
				trytrylib("RGSS101C.dll");
				trytrylib("RGSS101J.dll");
				trytrylib("RGSS100E.dll");
				trytrylib("RGSS100C.dll");
				trytrylib("RGSS100J.dll");
			}
			else
			{
				libname = trylib(arg);
			}
			if (libname) break;
		}
	}
	else
	{
		verbose(no library is given);
		verbose(searching for some famous library...);
		trytrylib("RGSS301.dll");
		trytrylib("RGSS300.dll");
		trytrylib("RGSS202E.dll");
		trytrylib("RGSS202C.dll");
		trytrylib("RGSS202J.dll");
		trytrylib("RGSS201E.dll");
		trytrylib("RGSS201C.dll");
		trytrylib("RGSS201J.dll");
		trytrylib("RGSS200E.dll");
		trytrylib("RGSS200C.dll");
		trytrylib("RGSS200J.dll");
		trytrylib("RGSS104E.dll");
		trytrylib("RGSS104C.dll");
		trytrylib("RGSS104J.dll");
		trytrylib("RGSS103E.dll");
		trytrylib("RGSS103C.dll");
		trytrylib("RGSS103J.dll");
		trytrylib("RGSS102E.dll");
		trytrylib("RGSS102C.dll");
		trytrylib("RGSS102J.dll");
		trytrylib("RGSS101E.dll");
		trytrylib("RGSS101C.dll");
		trytrylib("RGSS101J.dll");
		trytrylib("RGSS100E.dll");
		trytrylib("RGSS100C.dll");
		trytrylib("RGSS100J.dll");
	}
}

char*
fgetline(FILE* fp)
{
	orzsb* sb = orzsb_create();
	char line[1025];
	char* ret;
	char* p;
	do
	{
		fgets(line, 1025, fp);
		orzsb_print(sb, line);
	}
	while(strchr(line, '\n') == NULL && !feof(fp));
	
	ret = orzsb_build(sb);
	
	if ((p = strchr(ret, '\n')) != NULL)
		*p = '\0';
	
	orzsb_clean(sb);
	free(sb);
	
	return ret;
}

int
tryrc(char* fname)
{
	verbosef("rc: trying: %s\n", fname);
	if(exists(fname))
	{
		char* fakeargv[3];
		FILE *fp = fopen(fname, "r");
		fakeargv[0] = argv0;
		while(!feof(fp))
		{
			char* line = fgetline(fp);
			char* p;
			verbosef("rc: hit %s\n", line);
			
			optarg = 0;
			optind = 0;
			opterr = 1;
			fakeargv[1] = line;
			
			if ((p = strchr(line, ' ')) != NULL)
			{
				*p = '\0';
				fakeargv[2] = p + 1;
				parseoptions(3, fakeargv);
			}
			else
			{
				parseoptions(2, fakeargv);
			}
			// free(line); 
			// don't free cause getopts will save pointer to argv
		}
		fclose(fp);
		verbose(rc: done);
		return 0;
	}
	return 1;
}

void
decidearg(int argc, char **argv)
{
	opterr = 0;
	while (1)
 	{
 		int c;
		int option_index = 0;
		
		c = getopt_long (argc, argv, short_options, long_options, &option_index);
		
		if (c == -1)
			break;
		
		if (c == 'c')
			flag_rcname = optarg;
		
		if (c == 'C')
			flag_rc = 1;
	}
	
	if (flag_rc != 1)
	{
		if (flag_rcname)
		{
			verbose(rc: `-c` given);
			if(tryrc(flag_rcname) != 0)
			{
				error(rc: given rc not found);
				exit(1);
			}
		}
		else
		{
			verbose(rc: searching for famous rc);
			do
			{
				if(tryrc(".irgssrc") == 0) break;
				if(tryrc("irgssrc") == 0) break;
			}while(0);
		}
	}
	
	optarg = 0;
	optind = 0;
	opterr = 1;
	parseoptions(argc, argv);
	
	orzsb_printf(code_settings, "::IRGSS::VERBOSE = %d\n", flag_verbose);
	if (flag_verbose)
		codesettings("(::IRGSS::VAR[:irb_conf]||={})[:VERBOSE] = true");
}

int
main (int argc, char **argv)
{
	_wsetlocale(LC_ALL, L"chs");
	
	argv0 = argv[0];
	rgssinit = FALSE;
	flag_rc = 0;
	flag_rcname = 0;
	code_settings = orzsb_create();
	codesettings("::IRGSS::TOP_BINDING = binding");
	
	decidearg(argc, argv);
	decidelib(argc, argv);
	
	if (!libname)
	{
		error(lib: no valid library is found... exiting...);
		error(help: try `irgss --help` for futher information);
		exit(1);
	}
	
	runrgss(libname);
	
	exit (0);
}


#ifdef	__cplusplus
}
#endif