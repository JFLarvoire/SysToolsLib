/*****************************************************************************\
*                                                                             *
*   File name	    MsgBox.c						      *
*                                                                             *
*   Description	    Display a message box 				      *
*                                                                             *
*   Notes	    This program must be a console application.		      *
*                   If it were linked as a Windows application, the command   *
*                   interpretor would not wait for its completion before      *
*                   executing the next command, thus preventing the use of    *
*                   exit codes.						      *
*                                                                             *
*                   I left in source for routine messagefx(), its associated  *
*                   routines, and the now useless invokation code.	      *
*		    They're compiled-out with #if defined(USEHIDDENWINDOW).   *
*                   The reason I did not remove it is that this code now has  *
*                   simplifications and improvements compared to the initial  *
*                   version in ksetup.c.		                      *
*                                                                             *
*                   When compiled in debug mode (_DEBUG defined),             *
*                   option -debug allows to log messages to the debugger.     *
*                   Use for example NTInternal's DbgView.exe to display them. *
*                                                                             *
*   History                                                                   *
*    2001-09-18 JFL Created this program.				      *
*    2016-12-20 JFL Added library records to allow building in SysToolsLib.   *
*    2016-12-31 JFL Fixed _setargv for use in WIN64 programs.		      *
*                   Display help on stdout.                                   *
*    2017-06-28 JFL Fixed the link warning. No functional code change.	      *
*                                                                             *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DATE    "2016-12-31"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <windows.h>
#include <stdio.h>	/* For displaying help on the console */

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Comdlg32.lib")

#pragma warning(disable:4204)	/* Ignore the warning: nonstandard extension used : non-constant aggregate initializer */
#pragma warning(disable:4221)	/* Ignore the warning: nonstandard extension used : ... cannot be initialized using address of automatic variable ... */

#if defined(_WIN32) && !defined(_WIN64)
#define OS_NAME "WIN32"
#endif
#if defined(_WIN64)
#define OS_NAME "WIN64"
#endif

// #define USEHIDDENWINDOW

#define streq(s1, s2) (!lstrcmp(s1, s2))     /* Test if strings are equal */
#define streqi(s1, s2) (!lstrcmpi(s1, s2))   /* Idem, not case sensitive */

// Define WIN32 replacements for Standard C library functions
#define malloc(size) (void *)LocalAlloc(LMEM_FIXED, size)
#define strlwr CharLower
#define strcmp lstrcmp
#define strcpy lstrcpy
#define strcat lstrcat
#define strlen lstrlen
#define _tell(hf) _llseek(hf, 0, FILE_CURRENT)
#define _filelength(hf) ( 

#define NARGS 20

HINSTANCE ThisInstance(void)
    {
    return GetModuleHandle(NULL);
    }

/* Debug */

#ifdef _DEBUG
#define DEBUG_VERSION "DEBUG"
#define DebugMsg(msg) MessageBox(NULL, msg, "Debug", MB_OK | MB_TOPMOST)
#define DebugMsg1(msg, arg) MessageBoxF("Debug", MB_OK | MB_TOPMOST, msg, arg)

#include "MsgNames.h"

void _cdecl OutputDebugF(const char *pszFormat, ...)
    {
    char szLine[1024];
    va_list pArgs;
    int n;

    va_start(pArgs, pszFormat);
    n = wvsprintf(szLine, pszFormat, pArgs);	// Non-portable. pArgs not guarantied to point to the next argument.
    va_end(pArgs);

    OutputDebugString(szLine);
    
    return;
    }
#else
#define DEBUG_VERSION
#endif // defined(_DEBUG)

/* Global variables */

#if _DEBUG && HAS_MSVCLIBX
extern int iDebug;
#else
int iDebug = 0;
#endif

/* Forward references */

void usage(void);
int IsSwitch(char *pszArg);
int BreakArgLine(LPSTR fpParms, char *ppszArg[], int iMaxArgs);
int _cdecl MessageBoxF(char *pszTitle, UINT uiStyle, const char *pszFormat, ...);
#if defined(USEHIDDENWINDOW)
HWND _cdecl messagefx(HWND hWnd, const char *pszFormat, ...);
int messagefxEnd(HWND *phWnd);
#endif // defined(USEHIDDENWINDOW)
int PromptWindow(const char *pszTitle, const char *pszPrompt, char *pszBuffer, int iLength);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    WinMain						      |
|									      |
|   Description	    Windows program main routine			      |
|									      |
|   Parameters	    HANDLE hInstance	    Current instance	 	      |
|		    HANDLE hPrevInstance    Previous instance		      |
|		    LPSTR lpszCmdLine       Command line		      |
|		    int iCmdShow            Show-window type (open/icon)      |
|									      |
|   Returns	    The return code to pass to the OS.			      |
|									      |
|   Notes	    This routine calls main.				      |
|									      |
|   History								      |
|    2001-09-26 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#if WINMAIN_NEEDED

#define main main_ // Avoid having a main label, to make sure we're a Windows app.
int main(int argc, char *argv[]);

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
    {
    char *argv[NARGS];
    int argc;

    // Break the argument line
    argc = BreakArgLine(lpszCmdLine, argv, NARGS);

    return main(argc, argv);
    }

#endif // WINMAIN_NEEDED

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description	    Program main routine				      |
|									      |
|   Parameters	    int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The return code to pass to the OS.			      |
|									      |
|   Notes	    This routine is renamed main_, else WinMain would not be  |
|		    processed.						      |
|									      |
|   History								      |
|    2001-09-18 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(USEHIDDENWINDOW)

HWND hWndHidden;	    // The hidden window that solves our problems

void __cdecl CleanupHiddenWindow(void)
    {
    if (hWndHidden) messagefxEnd(&hWndHidden);    // Destroy the hidden window.
    }

#endif // defined(USEHIDDENWINDOW)

int main(int argc, char *argv[])
    {
    int i;
    int iFormat = 0;
    char *pszFormat = NULL;
    char *pszTitle = "";
    UINT uiStyle = MB_OK | MB_TOPMOST | MB_TASKMODAL;
    int iRet;
    char *pszOpenFile = NULL;	// If not NULL, display open file dialog box, and store result in given output file.
    char *pszBatch = "MBResult.bat";
    char *pszPrompt = NULL;
    char szBatch[MAX_PATH];
    HANDLE hf;

    /* Process arguments */

    for (i=1; i<argc; i++)
        {
	if (IsSwitch(argv[i]))		    /* It's a switch */
            {
            strlwr(argv[i]);
	    if (streq(argv[i]+1, "?"))		/* -?: Help */
                {
		usage();
                return 0;
		}
	    if (streq(argv[i]+1, "b") || streq(argv[i]+1, "batch"))
                {
		if (((i+1) < argc) && !IsSwitch(argv[i+1])) 
		    {
		    pszBatch = argv[++i];
		    }
                continue;
		}
	    if (streq(argv[i]+1, "c") || streq(argv[i]+1, "cancel"))
                {
		uiStyle |= MB_OKCANCEL;
                continue;
		}
	    if (streq(argv[i]+1, "debug"))
                {
		iDebug = TRUE;
                continue;
		}
	    if (streq(argv[i]+1, "e") || streq(argv[i]+1, "edit"))
                {
		if (((i+1) < argc) && !IsSwitch(argv[i+1]))
		    pszPrompt = argv[++i];
		else
		    pszPrompt = "";
                continue;
		}
	    if (streq(argv[i]+1, "i") || streq(argv[i]+1, "information"))
                {
		uiStyle |= MB_ICONINFORMATION;
                continue;
		}
	    if (streq(argv[i]+1, "o") || streq(argv[i]+1, "openfile"))
                {
		if (((i+1) < argc) && !IsSwitch(argv[i+1])) 
		    pszOpenFile = argv[++i];
		else
		    pszOpenFile = "C:\\";
                continue;
		}
	    if (streq(argv[i]+1, "q") || streq(argv[i]+1, "question"))
                {
		uiStyle |= MB_ICONQUESTION;
                continue;
		}
	    if (streq(argv[i]+1, "s") || streq(argv[i]+1, "stop"))
                {
		uiStyle |= MB_ICONSTOP;
                continue;
		}
	    if (streq(argv[i]+1, "t") || streq(argv[i]+1, "title"))
                {
		if (((i+1) < argc) && !IsSwitch(argv[i+1])) pszTitle = argv[++i];
                continue;
		}
	    if (streq(argv[i]+1, "x") || streq(argv[i]+1, "exclamation"))
                {
		uiStyle |= MB_ICONEXCLAMATION;
                continue;
		}
	    // Unsupported switch!
	    // Fall through
            }
	iFormat = i;
	pszFormat = argv[i];
	break;		// All following arguments to be displayed
	}

    // Convert relative names to absolute before GetOpenFileName() changes the current directory.
    GetFullPathName(pszBatch, sizeof(szBatch), szBatch, NULL);  

#if defined(USEHIDDENWINDOW)
    // Create a hidden window. Its only reason for being is that its very existence
    // forces MessageBox() to wait for the closure of the message box before returning!
    // ~~JFL 2001-09-25 This does not seem necessary if the application is a console app instead of a Windows app.
    hWndHidden = messagefx(NULL, "MsgBox.exe helper window", SW_HIDE, "You should not see this...");
    atexit(CleanupHiddenWindow);
#endif // defined(USEHIDDENWINDOW)

    /* Case of OpenFile request */

    if (pszOpenFile)
	{
	char szSelected[MAX_PATH+13] = "SET MBRESULT=";
	OPENFILENAME ofn = 
	    {
	    sizeof(OPENFILENAME),	//  DWORD         lStructSize; 
	    NULL,			//  HWND          hwndOwner; 
	    ThisInstance(),		//  HINSTANCE     hInstance; 
	    NULL,			//  LPCTSTR       lpstrFilter; 
	    NULL,			//  LPTSTR        lpstrCustomFilter; 
	    0,				//  DWORD         nMaxCustFilter; 
	    0,				//  DWORD         nFilterIndex; 
	    szSelected+13,		//  LPTSTR        lpstrFile; 
	    sizeof(szSelected)-13,	//  DWORD         nMaxFile; 
	    NULL,			//  LPTSTR        lpstrFileTitle; 
	    0,				//  DWORD         nMaxFileTitle; 
	    pszOpenFile,		//  LPCTSTR       lpstrInitialDir; 
	    pszTitle,			//  LPCTSTR       lpstrTitle; 
	    OFN_LONGNAMES,		//  DWORD         Flags; 
	    0,				//  WORD          nFileOffset; 
	    0,				//  WORD          nFileExtension; 
	    NULL,			//  LPCTSTR       lpstrDefExt; 
	    0,				//  LPARAM        lCustData; 
	    NULL,			//  LPOFNHOOKPROC lpfnHook; 
	    NULL,			// LPCTSTR       lpTemplateName; 
	    };
	BOOL bResult;

	// Launch the Open File dialog box.
	bResult = GetOpenFileName(&ofn);	// Open the standard dialog box for finding files.
	if (!bResult)
	    {
	    DWORD dwErr = CommDlgExtendedError();
	    MessageBoxF("MsgBox.exe debug error", MB_OK | MB_TOPMOST, "Returned err 0x%x.\n", dwErr);
	    exit(252);
	    }
	// MessageBoxF("MsgBox.exe debug", MB_OK | MB_TOPMOST, "Returned 0x%x for File %s\n", bResult, szSelected);

	// Output the result into a batch file to call as a subroutine of the parent batch.
	hf = CreateFile(szBatch,	// file name
			GENERIC_READ | GENERIC_WRITE,    // access mode
			0,              // share mode
			NULL,		// SD
			CREATE_ALWAYS,  // how to create: Erase if already existing.
			FILE_ATTRIBUTE_NORMAL,	// file attributes
			NULL);          // handle to template file
	if (hf != INVALID_HANDLE_VALUE) 
	    { 
	    DWORD dw;
	    WriteFile(hf, szSelected, strlen(szSelected), &dw, NULL); 
	    WriteFile(hf, "\r\n", 2, &dw, NULL); 
	    CloseHandle(hf);
	    }
	else
	    {
	    MessageBoxF("MsgBox.exe error", MB_OK | MB_TOPMOST, "Cannot open file %s\n", szBatch);
	    }

	exit(0);
	}

    // Other cases require a message string/

    if (!pszFormat) 
	{
	usage();
	return 255;
	}

    /* Case of Prompt Window */

    if (pszPrompt)
	{
	char szBuf[256] = "SET MBRESULT=";
	int iStart = strlen(szBuf);
	strcat(szBuf, pszPrompt);

	iRet = PromptWindow(pszTitle, pszFormat, szBuf+iStart, sizeof(szBuf)-iStart);

	// Output the result into a batch file to call as a subroutine of the parent batch.
	hf = CreateFile(szBatch,	// file name
			GENERIC_READ | GENERIC_WRITE,    // access mode
			0,              // share mode
			NULL,		// SD
			CREATE_ALWAYS,  // how to create: Erase if already existing.
			FILE_ATTRIBUTE_NORMAL,	// file attributes
			NULL);          // handle to template file
	if (hf != INVALID_HANDLE_VALUE) 
	    { 
	    DWORD dw;
	    WriteFile(hf, szBuf, strlen(szBuf), &dw, NULL); 
	    WriteFile(hf, "\r\n", 2, &dw, NULL); 
	    CloseHandle(hf);
	    }
	else
	    {
	    MessageBoxF("MsgBox.exe error", MB_OK | MB_TOPMOST, "Cannot open file %s\n", szBatch);
	    }

	exit(iRet);
	}

    /* Default: Standard message box */

    if (pszFormat[0] == '@')
	{
	long lSize;
	HFILE hFile;

	hFile = _lopen(pszFormat+1, OF_READ);
	if (hFile == HFILE_ERROR)
	    {
	    MessageBoxF("MsgBox.exe Error", MB_OK | MB_TOPMOST, "Cannot open file %s.\n", pszFormat+1);
	    return 254;
	    }
	// Get the file size.
	{
	long lPos = _llseek(hFile, 0, FILE_CURRENT);	// Current position
	lSize = _llseek(hFile, 0, FILE_END);		// Find the position of the end
	_llseek(hFile, lPos, FILE_BEGIN);		// Return to the initial position
	}

	pszFormat = malloc(lSize);
	if (!pszFormat)
	    {
	    MessageBoxF("MsgBox.exe Error", MB_OK | MB_TOPMOST, "Not enough memory.\n");
	    return 253;
	    }
	_lread(hFile, pszFormat, lSize);
	_lclose(hFile);
	}

    i = iFormat;
    iRet = MessageBoxF(pszTitle, uiStyle, pszFormat, argv[i+1], argv[i+2], argv[i+3], argv[i+4], argv[i+5], argv[i+6], argv[i+7], argv[i+8], argv[i+9]);
    iRet -= 1;	// Change from 1-based to 0-based. 0=OK.

    /* Terminate, reporting success */

    return iRet;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    usage						      |
|									      |
|   Description	    Display a brief help for this program		      |
|									      |
|   Parameters	    None						      |
|									      |
|   Returns	    N/A 						      |
|                                                                             |
|   History								      |
|    1994-09-09 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void)
    {
#if 0
    MessageBoxF("Usage", MB_OK | MB_TOPMOST,
#else
    printf("%s",
"MsgBox version " PROGRAM_DATE " " OS_NAME " " DEBUG_VERSION "\n\
\n\
Usage:\n\
\n\
"
#endif
"\
[start /b [/w]] MsgBox [switches] {message}\n\
\n\
Optional switches:\n\
  -b {bat}	Set output batch name for opts -e, -o. Default: MBResult.bat.\n\
		Content: \"set MBRESULT=string\".\n\
  -c		Add a cancel button.\n\
  -e [init]	Add an edit box below the message. Default initial value: \"\".\n\
                Creates a batch file with the input text. See option -b.\n\
  -i		Add an (i)nformation icon.\n\
  -o [init]	Display an Open File dialog box. Default initial path: C:\\.\n\
                Creates a batch file with the selected path. See option -b.\n\
  -q		Add a question-mark icon.\n\
  -s		Add a stop-sign icon.\n\
  -t \"title\"	String to put on the title bar.\n\
  -x		Add an exclamation-point icon.\n\
\n\
Message:\n\
  \"a string to display\"	Use \\n or \\xXX for special characters.\n\
  \"a %%s found\" {string}	%%s replaced by the {string} argument.\n\
  @inputfile		Display contents from this file.\n\
\n\
ErrorLevel return value:\n\
  0=OK  1=Cancel  2=Abort  3=Retry  4=Ignore  5=Yes  6=No  7=Close  8=Help\n\
");

    return;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    IsSwitch						      |
|									      |
|   Description	    Test if a command line argument is a switch.	      |
|									      |
|   Parameters	    char *pszArg					      |
|									      |
|   Returns	    TRUE or FALSE					      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    1997-03-04 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg)
    {
    switch (*pszArg)
	{
	case '-':
	case '/':
	    return TRUE;
	default:
	    return FALSE;
	}
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    BreakArgLine                                              |
|                                                                             |
|   Description	    Break the DOS command line into standard C arguments      |
|                                                                             |
|   Parameters	    LPSTR fpParms       NUL-terminated argument line          |
|                   char *pszArg[]      Array of arguments pointers           |
|                   int iMaxArgs        Number of elements in pszArg          |
|                                                                             |
|   Returns         int argc            Number of arguments found             |
|                                                                             |
|   Notes           This routine does not set argv[0] to the program name.    |
|                                                                             |
|   History								      |
|    1993-10-05 JFL Initial implementation within devmain().		      |
|    1994-04-14 JFL Extracted from devmain, and created this routine.	      |
|    1995-04-07 JFL Extracted from llkinit.c.				      |
|    1996-09-26 JFL Adapted to Win32 programs.				      |
|    1996-12-11 JFL Use Windows string routines.			      |
|    2001-09-18 JFL Set argv[0] with actual module file name.		      |
|                   Manage quoted strings as a single argument.               |
|    2001-09-25 JFL Only process \x inside strings.                           |
*                                                                             *
\*---------------------------------------------------------------------------*/

int BreakArgLine(LPSTR fpParms, char *ppszArg[], int iMaxArgs)
    {
    int i, j;
    int argc;
    char c, c0;
    char *pszCopy;
    int iString = FALSE;
    int iQuote = FALSE;
    static char szModule[MAX_PATH] = {0};

    if (!szModule[0]) GetModuleFileName(NULL, szModule, sizeof(szModule));
    ppszArg[0] = szModule;

    /* Make a local copy of the argument line */
    /* Break down the local copy into standard C arguments */

    pszCopy = malloc(lstrlen(fpParms) + 1);
    // Copy the string, managing quoted characters
    for (i=0, j=0, iQuote = FALSE, argc=1, c0='\0'; argc<iMaxArgs; i++)
	{
	c = fpParms[i];
	if (!c)		    // End of argument line
	    {
	    pszCopy[j++] = c;
	    break;
	    }
	if (iQuote)
	    {
	    iQuote = FALSE;
	    switch (c)
		{
		char *pc;
		case 'n': c = '\n'; break;
		case 'r': c = '\r'; break;
		case 'x': c = (char)strtoul(fpParms+i+1, &pc, 16); i = (int)(pc-fpParms); i-=1; break;
		default: break;	// Ignore others, including \ and ".
		}
	    }
	else
	    {
	    if (iString && (c == '\\'))	    // Quoted character in string
		{
		iQuote = TRUE; 
		continue;
		}
	    if (c == '"')
		{
		iString = !iString;
		if (iString) continue;
		c = '\0';   // Force end of argument at end of string.
		if (!c0)    // Case of empty string: Define an empty argument.
		    {
		    pszCopy[j] = '\0';
		    ppszArg[argc] = pszCopy+j;
		    argc += 1;
		    j += 1;
		    continue;
		    }
		}
	    }
	if ((!iString) && ((c == ' ') || (c == '\t'))) c = '\0';
	pszCopy[j] = c;
        if (c && !c0)
            {
            ppszArg[argc] = pszCopy+j;
            argc += 1;
            }
        c0 = c;
	j += 1;
	}
    
    return argc;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    _setargv						      |
|									      |
|   Description	    Msft standard CRT routine for parsing the command line.   |
|									      |
|   Parameters	    char *_acmdln	Command line parameters.	      |
|									      |
|   Returns	    __argc = Number of arguments			      |
|		    __argv = Array of arguments				      |
|		    _pgmptr = The program pathname			      |
|									      |
|   Notes	    When linked in, replaces the default routine from the CRT.|
|		    							      |
|   History								      |
|    2001-09-25 JFL Created this routine				      |
|    2016-12-31 JFL Changed the return type from void to int, else the WIN64  |
|                   version fails with message:				      |
|		    runtime error R6008 - not enough space for arguments      |
*									      *
\*---------------------------------------------------------------------------*/

#ifndef main

_CRTIMP extern char *_acmdln;

int __cdecl _setargv(void)
    {
    static char *argv[NARGS];

    // Break the argument line
    __argc = BreakArgLine(_acmdln, argv, NARGS) - 1;
    __argv = argv+1;	// _acmdln has a copy of the program name.
    _pgmptr = argv[0];

    return 0;
    }

#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    MessageBoxF						      |
|									      |
|   Description	    Display a formatted line in a message box.                |
|									      |
|   Parameters	    char *pszTitle	String to display in the title bar.   |
|		    UINT uiStyle	Window style			      |
|		    Then same as printf					      |
|									      |
|   Returns	    MessageBox return value				      |
|									      |
|   Notes	    Output limited to 1024 characters.			      |
|									      |
|		    MessageBox() returns immediately if no "normal" window is |
|		    opened in the application. Use messagefx() to create one  |
|		    if needed.						      |
|									      |
|   History								      |
|    2001-09-18 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl MessageBoxF(char *pszTitle, UINT uiStyle, const char *pszFormat, ...)
    {
    char szLine[1024];
    va_list pArgs;
    int n;

    va_start(pArgs, pszFormat);
    n = wvsprintf(szLine, pszFormat, pArgs);	// Non-portable. pArgs not guarantied to point to the next argument.
    va_end(pArgs);

    return MessageBox(NULL, szLine, pszTitle, uiStyle);
    }

#if defined(USEHIDDENWINDOW)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    messagefx						      |
|									      |
|   Description	    Display a formatted line in a persistent window	      |
|									      |
|   Parameters	    HWND hWnd		The window. If NULL, create one.      |
|		    char *pszFormat	Output format string		      |
|		    ... 		Optional arguments, same as printf.   |
|									      |
|   Returns	    The window handle					      |
|									      |
|   Notes	    Output limited to 1024 characters.			      |
|									      |
|		    Use messagefx(NULL, "title", iShow, "message 1", ...) to  |
|			         create the window.			      |
|		    Use messagefx(hWnd, "message 2") to update its contents.  |
|		    Use messagefxEnd(&hWnd) to close the window & clear hWnd. |
|									      |
|   History								      |
|    1998-11-25 JFL Created this routine				      |
|    2001-09-19 JFL Adapted from ksetup.c. Rewritten to make it generic.      |
|		    Added "title" and iShow arguments on creation call.	      |
*									      *
\*---------------------------------------------------------------------------*/

static LRESULT CALLBACK messagefxWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    static char szLine[1024];
    char *pc;
    char *pc0;
    int iLen;

    switch (uMsg) {
	case WM_CREATE:
	    lstrcpy(szLine, ((LPCREATESTRUCT)lParam)->lpCreateParams);
	    break;

	case WM_USER:	    // Display another message in the same window.
	    lstrcpy(szLine, (char *)lParam);
	    InvalidateRect(hWnd, NULL, TRUE);
	    UpdateWindow(hWnd);
	    return 0;

        case WM_PAINT:
	    {
	    HDC hDC;			/* display-context variable  */
	    PAINTSTRUCT ps;		/* paint structure	     */
	    int x, y;
	    TEXTMETRIC tm;

	    /* Set up a display context to begin painting */
	    hDC = BeginPaint (hWnd, &ps);
	    SetBkColor(hDC, GetSysColor(COLOR_MENU));

	    GetTextMetrics(hDC, &tm);
	    x = tm.tmMaxCharWidth;
	    y = tm.tmHeight;

	    for (pc=pc0=szLine; pc; pc0=pc+1)	// For every line
		{
		pc = strchr(pc0, '\n'); 	// Search end of line
		if (pc)
		    iLen = pc-pc0;
		else
		    iLen = lstrlen(pc0);
		TextOut(hDC, x, y, pc0, iLen);
		y += tm.tmHeight;
		}

            /* Tell Windows you are done painting */
            EndPaint (hWnd,  &ps);

	    return 0;
	    }

	default:
	    break;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

HWND _cdecl messagefx(HWND hWnd, const char *pszFormat, ...)
    {
    char szLine[1024];
    va_list pArgs;
    int n;
    WNDCLASS wc;
    static BOOL bRegistered = FALSE;
    int iShow;
    const char *pszTitle;

    // Create our Window style (To be done only once)
    if (!bRegistered)
	{
	wc.style = 0;
	wc.lpfnWndProc = messagefxWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = ThisInstance();
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_MENU+1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "MfxWindowClass";

	RegisterClass(&wc);

	bRegistered = TRUE;
	}

    // Extract command line arguments
    va_start(pArgs, pszFormat);
    if (!hWnd)
	{
	pszTitle = pszFormat;
	// Extract window creation parameters
	iShow = va_arg(pArgs, int);
	pszFormat = va_arg(pArgs, char *);
	}
    // Format the line to output
    n = wvsprintf(szLine, pszFormat, pArgs);	// Non-portable. pArgs not guarantied to point to the next argument.
    va_end(pArgs);

    // Create a message window
    if (!hWnd)
	{
	hWnd = CreateWindow(
		"MfxWindowClass",   // pointer to registered class name
		pszTitle,	    // pointer to window name
		WS_BORDER | WS_CAPTION | WS_DISABLED | WS_POPUP, // window style
	        (GetSystemMetrics(SM_CXFULLSCREEN) - 250)/2, // horizontal position of window: Centered
	        (GetSystemMetrics(SM_CYFULLSCREEN) - 100)/2, // vertical position of window: Centered
		250,		    // window width
		100,		    // window height
		NULL,		    // handle to parent or owner window
		NULL,		    // handle to menu or child-window identifier
		ThisInstance(),	    // handle to application instance
		szLine);	    // pointer to window-creation data
	ShowWindow(hWnd, iShow);
	UpdateWindow(hWnd);
	}
    else
	{
	SendMessage(hWnd, WM_USER, 0, (LPARAM)szLine);
	}

    return hWnd;
    }

int messagefxEnd(HWND *phWnd)
    {
    int iErr = 0;

    if (*phWnd) iErr = !DestroyWindow(*phWnd);
    *phWnd = NULL;
    return iErr;
    }

#endif // defined(USEHIDDENWINDOW)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    Get message area					      |
|									      |
|   Description	    Compute the necessary pixel area to display a text.	      |
|									      |
|   Parameters	    HWND hWnd		The window to display in.             |
|		    char *pszMsg	The string to display. May contain \n.|
|		    PSIZE pSize		Output size structure.		      |
|									      |
|   Returns	    Nothing          					      |
|									      |
|   Notes	    Function GetTextExtentPoint32() does not manage multiple  |
|		    lines of text. So we had to devise ou own.		      |
|									      |
|		    This routine predicts the width based on the average      |
|		    character width. In some cases, the returned value may    |
|		    not be sufficient!					      |
|									      |
|   History								      |
|    2001-09-24 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void GetMessageArea(HWND hWnd, char *pszMsg, PSIZE pSize)
    {
    HDC hDC;
    PAINTSTRUCT ps;		/* paint structure	     */
    int x, y;
    int xMax;
    char *pc0;
    char *pc;
    TEXTMETRIC tm;
    int iLen;

    /* Set up a display context to the window */
    hDC = BeginPaint (hWnd, &ps);
    GetTextMetrics(hDC, &tm);

    x = 0;
    y = 0;
    xMax = 0;

    for (pc=pc0=pszMsg; pc; pc0=pc+1)	// For every line
	{
	pc = strchr(pc0, '\n'); 	// Search end of line
	if (pc)
	    iLen = (int)(pc-pc0);
	else
	    iLen = lstrlen(pc0);
	x = iLen * tm.tmAveCharWidth;
	if (x > xMax) xMax = x;
	y += tm.tmHeight;
	}

    EndPaint (hWnd,  &ps);

    pSize->cx = xMax;
    pSize->cy = y;

    return;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    PromptWindow					      |
|									      |
|   Description	    Display prompt and an edit field in a message box	      |
|									      |
|   Parameters	    const char *pszTitle    The title. If "", no title.       |
|		    const char *pszPrompt   The question.		      |
|		    char *pszBuffer	    Input: Edit field default content.|
|					    Output: What the user typed in.   |
|		    int iLength		    The buffer length.		      |
|									      |
|   Returns	    0 = OK						      |
|									      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2001-09-20 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#define PWWIDTH 250	// Prompt Window minimal width. Later adjusted to fit text if needed.
#define PWHEIGHT 150	// Prompt Window minimal height. Later adjusted to fit text if needed.
#define CYEDIT 25	// Edit field height.
#define CXBUTTON 77	// OK button width.
#define CYBUTTON 27	// OK button height.

typedef struct PromptWindowExtras
    {
    LONG_PTR hEdit;	    // Edit control child window
    LONG_PTR pszOutBuf;	    // Output buffer for the input text
    long lBufSize;	    // Size of the output buffer
    int iExitCode;	    // The window exit code
    } PromptWindowExtras, *PPWE;
#define PWEOFFSET(item) ((char *)&(((PPWE)NULL)->item) - (char *)&(((PPWE)NULL)->hEdit))

#define ID_CHILDEDIT 101
#define ID_OK 102

static LRESULT CALLBACK PromptWindowProc1(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK PromptWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
#ifdef _DEBUG
    char szMsgName[64];
    LRESULT lResult;
    static int iShift = 0;

    if (iDebug) OutputDebugF("PromptWindowProc[%d](0x%x, %s, 0x%x, 0x%x)\n", iShift, hWnd, GetMsgName(uMsg, szMsgName, sizeof(szMsgName)), wParam, lParam);

    iShift++;

    lResult = PromptWindowProc1(hWnd, uMsg, wParam, lParam);

    iShift--;

    if (iDebug) OutputDebugF("PromptWindowProc[%d]() returns 0x%x\n", iShift, lResult);

    return lResult;
    }

static LRESULT CALLBACK PromptWindowProc1(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
#endif // defined(_DEBUG)
    static char szLine[1024] = "Le commentaire";
    char *pc;
    char *pc0;
    int iLen;
    HWND hWndEdit = (HWND)GetWindowLongPtr(hWnd, PWEOFFSET(hEdit));
    char *pszBuf = (char *)GetWindowLongPtr(hWnd, PWEOFFSET(pszOutBuf));
    HWND hWndButton;
    SIZE size;
    long lyEdit;
    long lyButton;
    NONCLIENTMETRICS ncm;

    // char szMsgName[64];
    // OutputDebugF("PromptWindowProc(%d, %s, 0x%x, 0x%x)\n", hWnd, GetMsgName(uMsg, szMsgName, sizeof(szMsgName)), wParam, lParam);

    switch (uMsg) 
	{
	case WM_CREATE:
	    lstrcpy(szLine, ((LPCREATESTRUCT)lParam)->lpCreateParams);

	    GetMessageArea(hWnd, szLine, &size);
	    ncm.cbSize = sizeof(NONCLIENTMETRICS);
	    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);

	    size.cx += ncm.iBorderWidth;
	    size.cx += 10;  // 5 pixels margins on both sides
	    size.cx += ncm.iBorderWidth;
	    size.cx += 30;  // ???
	    if (size.cx < PWWIDTH) size.cx = PWWIDTH;

	    size.cy += ncm.iBorderWidth;;
	    size.cy += ncm.iCaptionHeight; // Title bar heigth
	    size.cy += 5;
	    lyEdit = size.cy;
	    size.cy += CYEDIT;
	    size.cy += 5;
	    lyButton = size.cy;
	    size.cy += CYBUTTON;
	    size.cy += 5;
	    size.cy += ncm.iBorderWidth;;
	    size.cy += 25;  // ???

	    MoveWindow(hWnd,      // handle to window
		       (GetSystemMetrics(SM_CXFULLSCREEN) - size.cx)/2, // horizontal position of window: Centered
		       (GetSystemMetrics(SM_CYFULLSCREEN) - size.cy)/2, // vertical position of window: Centered
		       size.cx,	    // window width
		       size.cy,	    // window height
		       FALSE);	    // repaint option: Don't.
	    
	    // Create the edit control
            hWndEdit = CreateWindow( 
                "EDIT",     // predefined class 
                NULL,       // no window title 
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 
                5, lyEdit, size.cx-25, CYEDIT, // set size in WM_SIZE message 
                hWnd,       // parent window 
                (HMENU)ID_CHILDEDIT, // edit control ID 
                /* (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), */ /* Not supported by WIN64 */
                (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);                // pointer not needed 
	    SetWindowLongPtr(hWnd, PWEOFFSET(hEdit), (LONG_PTR)hWndEdit); // Save the child window handle.
	    // Note: The default content is later set by the main thread.

	    // Create the OK button
	    hWndButton = CreateWindow(	
		"BUTTON",   // predefined class 
		"OK",       // button text 
		WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // styles 
 		// Size and position values are given explicitly, because 
		// the CW_USEDEFAULT constant gives zero values for buttons. 
		(size.cx - CXBUTTON)/2,         // starting x position: Centered
		lyButton,         // starting y position 
		CXBUTTON,        // button width 
		CYBUTTON,        // button height 
		hWnd,       // parent window 
		(HMENU)ID_OK,       // No menu 
                /* (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), */ /* Not supported by WIN64 */
                (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);      // pointer not needed 

	    break;

	case WM_COMMAND:
	    if (wParam == ID_OK) wParam = 1;   // User clicked [OK]. Do as if he pressed Enter.
	    if (   (wParam == 1)        // User pressed Enter
		|| (wParam == 2))	// User pressed ESC
		{
		SetWindowLong(hWnd, PWEOFFSET(iExitCode), (LONG)wParam-1);	// Update the exit code.
		*(WORD *)pszBuf = (WORD)GetWindowLong(hWnd, PWEOFFSET(lBufSize));
		SendDlgItemMessage(hWnd, 
		    ID_CHILDEDIT, 
		    EM_GETLINE, 
		    (WPARAM) 0,       // line 0 
		    (LPARAM) pszBuf); 
		DestroyWindow(hWnd);
		return wParam;
		}
	    break;

	case WM_DESTROY:
	    // SetEvent(hEvent);	    // Unblock the main thread
	    PostQuitMessage(GetWindowLong(hWnd, PWEOFFSET(iExitCode)));
	    break;

        case WM_SETFOCUS: 
            SetFocus(hWndEdit); 
            return 0; 

        case WM_PAINT:
	    {
	    HDC hDC;			/* display-context variable  */
	    PAINTSTRUCT ps;		/* paint structure	     */
	    int x, y;
	    TEXTMETRIC tm;

	    /* Set up a display context to begin painting */
	    hDC = BeginPaint (hWnd, &ps);
	    SetBkColor(hDC, GetSysColor(COLOR_MENU));

	    GetTextMetrics(hDC, &tm);
	    x = 5;
	    y = tm.tmHeight / 2;

	    for (pc=pc0=szLine; pc; pc0=pc+1)	// For every line
		{
		pc = strchr(pc0, '\n'); 	// Search end of line
		if (pc)
		    iLen = (int)(pc-pc0);
		else
		    iLen = lstrlen(pc0);
		TextOut(hDC, x, y, pc0, iLen);
		y += tm.tmHeight;
		}

            /* Tell Windows you are done painting */
            EndPaint (hWnd,  &ps);

	    return 0;
	    }

	default:
	    break;
	}

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

int PromptWindow(const char *pszTitle, const char *pszPrompt, char *pszBuffer, int iLength)
    {
    WNDCLASS wc;
    static BOOL bRegistered = FALSE;
    HWND hWnd;
    MSG msg;

    // Create our Window style (To be done only once)
    if (!bRegistered)
	{
	wc.style = 0;
	wc.lpfnWndProc = PromptWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(PromptWindowExtras);
	wc.hInstance = ThisInstance();
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_MENU+1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "PromptWindowClass";

	RegisterClass(&wc);

	bRegistered = TRUE;
	}

    // Create a Prompt window
    hWnd = CreateWindow(
	    "PromptWindowClass",   // pointer to registered class name
	    pszTitle,	    // pointer to window name
	    WS_BORDER | WS_POPUP | WS_DISABLED | WS_CAPTION, // window style
	    (GetSystemMetrics(SM_CXFULLSCREEN) - PWWIDTH)/2, // horizontal position of window: Centered
	    (GetSystemMetrics(SM_CYFULLSCREEN) - PWHEIGHT)/2, // vertical position of window: Centered
	    PWWIDTH,		    // window width
	    PWHEIGHT,		    // window height
	    NULL,		    // handle to parent or owner window
	    NULL,		    // handle to menu or child-window identifier
	    ThisInstance(),	    // handle to application instance
	    (LPVOID)pszPrompt);	    // pointer to window-creation data
    SetWindowLongPtr(hWnd, PWEOFFSET(pszOutBuf), (LONG_PTR)pszBuffer);
    SetWindowLong(hWnd, PWEOFFSET(lBufSize), (LONG_PTR)iLength);
    SetWindowLong(hWnd, PWEOFFSET(iExitCode), 1);   // Assume failure
    // Add default text to the window. 
    SendDlgItemMessage(hWnd, ID_CHILDEDIT, WM_SETTEXT, 0, (LPARAM)pszBuffer); 
    EnableWindow(hWnd, TRUE);	    // Allow user input
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Message loop: Forward messages to the dialog box.
    while (GetMessage(&msg, NULL, 0, 0)) // Repeat until we receive WM_QUIT...
	{ 
        // char szMsgName[64];
	// OutputDebugF("Message loop: msg = %s\n", GetMsgName(msg.message, szMsgName, sizeof(szMsgName)));
	if (!IsDialogMessage(hWnd, &msg)) 
	    { 
	    TranslateMessage(&msg); 
	    DispatchMessage(&msg); 
	    } 
	}

    return (int)(msg.wParam);	    // Return the exit code from the window (0 to 8)
    }

