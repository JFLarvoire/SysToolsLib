/*****************************************************************************\
*                                                                             *
*   File name:	    driver.c						      *
*									      *
*   Description:    Win32 driver management routines for NT and Win9x.	      *
*									      *
*   Notes:	    The WinNT code works reliably and is useful.	      *
*		    The Win9x code does not work well, and is left in for     *
*		    the unlikely case if someone wants to help debug it.      *
*		    							      *
*		    For Win9x's VxD drivers management, I have also made a    *
*		    16-bits vxd.com application, which works reliably.        *
*		    The 32-bits code here is experimental, and incomplete.    *
*		    See notes in routine ListVxDs about possible directions.  *
*		    							      *
*		    Use Visual Studio 2005 or older to generate a driver.exe  *
*		    that works in Windows 9x and NT4.			      *
*		    							      *
*		    To do: Finish porting all NT functions to using UTF8.     *
*		    (Although the current version works well in practice,     *
*		     as driver names seem to be ASCII only on French versions *
*		     of Windows, even though the description strings do use   *
*		     lots of non-ASCII accentuated characters.)		      *
*		    							      *
*   History:								      *
*    1998-04-09 JFL Created this program.				      *
*    2005-12-19 JFL Updated ListNtDrivers to display error messages if needed.*
*		    Updated list alignment to make it look better.	      *
*    2014-12-17 JFL Bug fix: NT Driver list failed if the buffer size         *
*                   requirement increased between the 1st call and the 2nd.   *
*		    Renamed option -d as -s for status.			      *
*		    Added option -d for debug.				      *
*		    Default action: List drivers			      *
*		    Version 1.2.					      *
*    2015-01-09 JFL Changed this to an MsvcLibX-based UTF-8 application.      *
*		    Version 2.0 Alpha.					      *
*    2015-01-15 JFL Added ListVxds routine placeholder, to avoid an error     *
*		    when invoking option -s in Windows 9x.		      *
*    2016-03-31 JFL Restructured the main routine.                            *
*		    Renamed option -s again as -e to enumerate drivers.	      *
*		    Added a new option -s to enumerate services.	      *
*		    Added an option -a to enumerate all, even stopped ones.   *
*		    Bug fix: The unload option was broken.		      *
*		    Bug fix: Several error cases were also broken.	      *
*		    Bug fix: Convert2Utf8() could overflow its buffers.	      *
*		    Version 2.1.					      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.2.1.1.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 2.1.2.      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Manage system drivers and services"
#define PROGRAM_NAME    "driver"
#define PROGRAM_VERSION "2.1.2"
#define PROGRAM_DATE    "2019-06-12"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#define _UTF8_SOURCE /* Tell MsvcLibX we're using UTF-8 strings */

#ifndef _WIN32
#error "This program only supports Windows"
#endif

/* #define WINVER 0x0400 /* Minimum OS target: Windows 95 and NT4 */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include <windows.h>
#include <winioctl.h>
#include <winnls.h>

/* The following include files are not available in the Microsoft C libraries */
/* JFL's MsvcLibX library extensions */
#include <iconv.h>		/* Code page conversion routines an variables */
/* SysToolsLib include files */
#include "debugm.h"	/* SysToolsLib debug macros */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

// NT Test
#if defined(_WIN32)
    #define IS_WIN32 TRUE
#else
    #define IS_WIN32 FALSE
#endif
#pragma warning(disable:4996)	/* Ignore the GetVersion() is deprecated warning */
#define IS_NT()    (IS_WIN32 && (BOOL)(GetVersion() < 0x80000000))
#define IS_WIN9x() (IS_WIN32 && (BOOL)(GetVersion() >= 0x80000000))

/* Pretend these are standard names, and avoid warnings about them being not */
#define kbhit _kbhit
#define getch _kbhit

/* My favorite definitions */

#define streq(s1, s2) (!strcmp(s1, s2))     /* Test if strings are equal */

#define WORD0(dw) (*((WORD *)(&(dw))+0))
#define WORD1(dw) (*((WORD *)(&(dw))+1))
#define BYTE0(dw) (*((BYTE *)(&(dw))+0))
#define BYTE1(dw) (*((BYTE *)(&(dw))+1))
#define BYTE2(dw) (*((BYTE *)(&(dw))+2))
#define BYTE3(dw) (*((BYTE *)(&(dw))+3))

/* Global variables */

int iVerbose = FALSE;
#define cp codePage			/* Initial console code page in MsvcLibX */

/* Forward references */

void usage(int retcode);
int PrintWin32Error(char *pszFormat, ...);
int IsSwitch(char *pszArg);
int MyAnsiToUtf8(char *pszString);
void _cdecl WaitForAnyKey(void);
int _cdecl oemprintf(const char *pszFormat, ...);
BOOL InstallNtDriver(char *pszServiceName, char *pszFileName, char *pszFriendlyName, int iStartType);
BOOL RemoveNtDriver(char *pszServiceName);
BOOL StartNtDriver(char *pszServiceName);
BOOL StopNtDriver(char *pszServiceName);
BOOL ListNtDrivers(DWORD dwType, DWORD dwState);
BOOL NtDriverStatus(char *pszServiceName);
BOOL InstallVxd(char *pszServiceName, char *pszFileName);
BOOL RemoveVxd(char *pszServiceName);
HANDLE LoadVxd(char *pszServicename);
BOOL UnloadVxd(char *pszDeviceName);
BOOL ListVxds(void);
BOOL GetVxdldrVersion(void);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    Program main initialization routine 		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|									      |
|    1998/03/12 JFL Initial implementattion.				      |
*									      *
\*---------------------------------------------------------------------------*/

enum actions {
  UNDEFINED = 0,
  ENUM,
  INSTALL,
  UNINSTALL,
  LOAD,
  UNLOAD,
  TEST
};

int main(int argc, char *argv[])
    {
    int i;
    int iAction = UNDEFINED;
    char *pszName = NULL;
    char *pszTest = NULL;
    int iWait = FALSE;
    BOOL bDone;
    DWORD dwType = SERVICE_DRIVER;
    DWORD dwState = SERVICE_ACTIVE;

    /* Process arguments */

    for (i=1; i<argc; i++) {
	char *arg = argv[i];
        if (IsSwitch(arg)) {		/* It's a switch */
            char *option = arg+1;		/* Skip the head - or / */
	    char *value = NULL;			/* Optional value following the switch */
	    if (((i+1)<argc) && !IsSwitch(argv[i+1])) value = argv[++i];
	    if (streq(option, "?")) {		/* -?: Help */
		usage(0);			     /* Display help & exit */
	    }
	    if (streq(option, "a")) {		/* -a: Enumerate all, even the stopped drivers */
		dwState = SERVICE_STATE_ALL;
		continue;
	    }
	    if (streq(option, "A")) {		/* -A: Force ANSI (CP 1252) output */
		cp = CP_ACP;
		continue;
	    }
	    DEBUG_CODE(
	    if (streq(option, "d")) {		/* -d: Debug mode */
		DEBUG_ON();
		continue;
	    }
	    )
	    if (streq(option, "e")) {		/* -s: Enumerate drivers */
		iAction = ENUM;
		pszName = value;
		dwType = SERVICE_DRIVER;
		continue;
	    }
	    if (streq(option, "i")) {		/* -i: Install the driver */
		if (!value) usage(1);
		iAction = INSTALL;
		pszName = value;
		continue;
	    }
	    if (streq(option, "l")) {		/* -l: Load */
		if (!value) usage(1);
		iAction = LOAD;
		pszName = value;
		continue;
	    }
	    if (streq(option, "O")) {		/* -O: Force OEM (CP 437) output */
		cp = CP_OEMCP;
		continue;
	    }
	    if (streq(option, "r")) {		/* -r: Uninstall the driver */
		if (!value) usage(1);
		iAction = UNINSTALL;
		pszName = value;
		continue;
	    }
	    if (streq(option, "s")) {		/* -s: Enumerate services */
		iAction = ENUM;
		pszName = value;
		dwType = SERVICE_WIN32;
		continue;
	    }
	    if (IS_WIN9x() && streq(option, "t")) {	/* -t: Test Load */
		iAction = TEST;
		pszTest = "vxd.com -l hpmmkb.vxd";
		continue;
	    }
	    if (streq(option, "u")) {		/* -u: Unload */
		if (!value) usage(1);
		iAction = UNLOAD;
		pszName = value;
		continue;
	    }
	    if (streq(option, "U")) {		/* -U: Force UTF-8 (CP 65001) output */
		cp = CP_UTF8;
		continue;
	    }
	    if (streq(option, "v")) {		/* -v: Verbose */
		iVerbose = TRUE;
		continue;
	    }
	    if (streq(option, "V")) {		/* -V: Display this program version */
		puts(DETAILED_VERSION);
		exit(0);
	    }
	    if (streq(option, "VV")) {		/* -VV: Display VXDLDR version */
		if (IS_WIN9x()) GetVxdldrVersion();
		else printf("Error: Only applies to Windows 95/98\n");
		exit(0);
	    }
	    if (streq(option, "w")) {		/* -w: Wait before exiting */
		iWait = TRUE;
		continue;
	    }
	    oemprintf("Unrecognized switch %s. Ignored.\n", arg);
            continue;
	}
	/* If no action has been chosen yet, assume this is an information request for a particular driver */
	if (!iAction) {
	  iAction = ENUM;
	  pszName = arg;
	  continue;
	}

	/* We really don't know what to do with this argument */
	oemprintf("Unexpected argument: %s\nIgnored.\n", arg);
        break;  /* Ignore other arguments */
    }

    if (iWait) atexit(WaitForAnyKey);

    /* If no action specified, by default list drivers */
    if (!iAction) {
      iAction = ENUM;
    }

    DEBUG_CODE_IF_ON(
	{
	printf("Char 0x82 = '%c' (OEM's e')\n", '\x82');
	printf("Char 0xE9 = '%c' (ANSI's e')\n", '\xE9');
	printf("Chars 0xC3 0xA7 = '%c%c' (UTF8's e')\n", '\xC3', '\xA9');
	printf("IS_WIN32 = %d\n", IS_WIN32);
	printf("GetVersion() = %lx\n", (unsigned long)GetVersion());
	// if (IS_WIN9x()) GetVxdldrVersion();
	}
    )

    switch (iAction) {
    case INSTALL:
	{
	char szDriverPathname[FILENAME_MAX];
	char szDeviceName[FILENAME_MAX];
	char szDescription[FILENAME_MAX];
	char *pc;

	GetCurrentDirectory(FILENAME_MAX, szDriverPathname);
	strcat(szDriverPathname, "\\");
	strcat(szDriverPathname, pszName);
	pc = strrchr(pszName, '\\');
	if (!pc) pc = pszName;
	lstrcpy(szDeviceName, pc);
	pc = strchr(szDeviceName, '.');
	if (pc) *pc = '\0';
	sprintf(szDescription, "Device %s installed by driver.exe", szDeviceName);

	if (IS_NT())
	    bDone = InstallNtDriver(szDeviceName, szDriverPathname, szDescription,
//				SERVICE_DEMAND_START); // start type
				SERVICE_AUTO_START);   // start type
	else
	    bDone = InstallVxd(szDeviceName, szDriverPathname);

	if (bDone)
	    {
	    printf("%s installed.\n", szDeviceName);
	    }
	else
	    {
	    PrintWin32Error("Failed to install %s.\n", szDeviceName);
	    exit(1);
	    }
	}
	break;

    case TEST:
	{
	if (IS_WIN9x())
	    {
	    STARTUPINFO startInfo;
	    PROCESS_INFORMATION processInfo;

	    // WinExec("vxd.com -l hpmmkb.vxd", SW_HIDE);
	    oemprintf("Running %s\n", pszTest);
	    startInfo.cb = sizeof(STARTUPINFO);
	    GetStartupInfo(&startInfo);
	    startInfo.wShowWindow = SW_HIDE;    // Prevent the 16-bits app from showing up on screen
	    bDone = CreateProcess(NULL,                     // pointer to name of executable module
				  pszTest,		    // pointer to command line string
				  NULL,                     // pointer to process security attributes
				  NULL,                     // pointer to thread security attributes
				  TRUE,                     // handle inheritance flag
				  0,                        // creation flags
				  NULL,			    // pointer to new environment block
				  NULL,                     // pointer to current directory name
				  &startInfo,               // pointer to STARTUPINFO
				  &processInfo);            // pointer to PROCESS_INFORMATION
	    // Wait until the process terminates
	    if (bDone)
		{
		WaitForSingleObject(processInfo.hProcess, 2000);
		oemprintf("VXD.COM terminated.\n");
		}
	    else
		{
		PrintWin32Error("Failed to run \"%s\".\n", pszTest);
		}
	    }
	else
	    {
	    oemprintf("Option -t not supported under NT.\n");
	    }
	}
	break;

    case ENUM:
	{
	if (IS_NT())
	    {
	    if (pszName)
		{
		iVerbose = TRUE;
		NtDriverStatus(pszName);
		}
	    else
		{
		ListNtDrivers(dwType, dwState);
		}
	    }
	else
	    {
	    ListVxds();
	    }
	}
	break;

    case LOAD:
	{
	if (IS_NT())
	    bDone = StartNtDriver(pszName);
	else
	    bDone = (LoadVxd(pszName) != NULL);

	if (bDone)
	    {
	    printf("%s started.\n", pszName);
	    }
	else
	    {
	    PrintWin32Error("Failed to start %s.\n", pszName);
	    exit(1);
	    }
	}
	break;

    case UNLOAD:
	{
	oemprintf("Unloading %s\n", pszName);

	if (IS_NT())
	    bDone = StopNtDriver(pszName);
	else
	    bDone = UnloadVxd(pszName);

	if (bDone)
	    {
	    printf("%s stopped.\n", pszName);
	    }
	else
	    {
	    PrintWin32Error("Failed to stop %s.\n", pszName);
	    exit(1);
	    }
	}
	break;

    case UNINSTALL:
	{
	if (IS_NT())
	    bDone = RemoveNtDriver(pszName);
	else
	    bDone = RemoveVxd(pszName);

	if (bDone)
	    {
	    printf("%s uninstalled.\n", pszName);
	    }
	else
	    {
	    PrintWin32Error("Failed to uninstall %s.\n", pszName);
	    exit(1);
	    }
	}
	break;
    }

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    usage						      |
|									      |
|   Description:    Display a brief help for this program		      |
|									      |
|   Parameters:     int iRetcode	Exit code to pass to the OS	      |
|									      |
|   Returns:	    N/A 						      |
|                                                                             |
|   History:								      |
|									      |
|    1998/03/12 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(int retcode)
    {
    printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: driver [switches]\n\
\n\
Switches:\n\
\n\
  -a         	    Enumerate all drivers or services, even those stopped.\n\
  -A                Force encoding the output using the ANSI character set.\n"
#ifdef _DEBUG
"\
  -d		    Display debug information.\n"
#endif
"\
  -e [module]	    Enumerate started drivers. Default.\n\
  -i {name}	    Install the driver. It must be in driver.exe directory.\n");
    if (IS_WIN9x()) printf("\
  -l {module|name}  Load a driver. Name may be its pathname under Win95.\n");
    else printf("\
  -l {module}       Load a driver. It must have been already installed.\n");
    printf("\
  -O                Force encoding the output using the OEM character set.\n\
  -r {module}	    Uninstall a driver. Removes it from the registry.\n\
  -s [module]	    Enumerate started services.\n");
    if (IS_WIN9x()) printf("\
  -t		    Test using vxd.com to load HPMMKB.VxD.\n");
    printf("\
  -u {module}	    Unload a driver. Specify its device module name.\n\
  -U                Force encoding the output using the UTF-8 encoding.\n\
  -v		    Display verbose information.\n\
  -V		    Display this program version.\n\
  -w		    Wait before exiting.\n\
\n\
Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr\n\
");

    exit(retcode);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    Convert2Utf8					      |
|									      |
|   Description:    Convert a Unicode string to UTF-8 in place.		      |
|									      |
|   Parameters:     void *pwszString	WCHAR* for input, char* for output    |
|		    char *tmpBuf	Temporary buffer for the UTF-8 string |
|		    size_t lBuf		Size of tmpBuf			      |
|									      |
|   Returns:	    The number of characters written			      |
|									      |
|   Notes:	    Quick and dirty method for converting this prog to UTF-8. |
|		    Will crop the string if it does not fit in the input.     |
|		    							      |
|		    Gotcha: The converted string is likely to fit for         |
|		    european languages, using Unicode chars < \u800. But for  |
|		    asian languages, the converted string is likely to be     |
|		    cropped.						      |
|		    							      |
|		    To do: Redesign all routines using this Convert2Utf8,     |
|		    until we can get rid of it.				      |
|		    							      |
|   History:								      |
|    2015-01-09 JFL Created this routine				      |
|    2016-03-31 JFL Bug fix: The conversion could overflow the input string.  |
*									      *
\*---------------------------------------------------------------------------*/

int CropUtf8String(char *pszString, size_t n) {
  size_t i;

  pszString[n] = '\0';
  i = n-1;
  if (n && (pszString[i] & 0x80)) { /* If the last byte is part of a multi-byte char */
    /*
       Char. number range  |        UTF-8 octet sequence
	  (hexadecimal)    |              (binary)
       --------------------+---------------------------------------------
       0000 0000-0000 007F | 0xxxxxxx
       0000 0080-0000 07FF | 110xxxxx 10xxxxxx
       0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
       0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    */
    char lead = '\x80';
    char mask = '\xC0';
    while (!(pszString[i] & 0x40)) { /* Go back to the first byte in that char */
      i -= 1;
      (signed char)lead >>= 1;
      (signed char)mask >>= 1;
    }
    if ((pszString[i] & mask) != lead) { /* Then the number of trailing bytes is incorrect */
      n = i;
      pszString[n] = '\0'; /* Erase that partial character, and end the string there */
    }
  }
  return (int)n;
}

int Convert2Utf8(void *pwszString, char *tmpBuf, size_t lBuf) {
  size_t l = lstrlenW(pwszString);	/* Number of wide chars in the input Unicode string */
  size_t n = WideCharToMultiByte(CP_UTF8, 0, pwszString, (int)l+1, tmpBuf, (int)lBuf, NULL, NULL); /* Number of bytes in the UTF8 copy */
  if (!n) return 0; /* The conversion failed somehow. Don't go any further. */
  if (n > (2*(l+1))) { /* The converted string would overflow the input string buffer */
    n = CropUtf8String(tmpBuf, 2*l + 1) + 1;
  }
  CopyMemory(pwszString, tmpBuf, n);
  return (int)n;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    PrintWin32Error					      |
|									      |
|   Description:    Display an error message, and the last win32 error text.  |
|									      |
|   Parameters:     char *pszFormat	Format string			      |
|		    ... 		Optional parameters		      |
|									      |
|   Returns:	    The number of characters written			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1997-03-25 JFL Created this routine.				      |
|    2015-01-15 JFL Converted to using UTF-8.				      |
*									      *
\*---------------------------------------------------------------------------*/

int PrintWin32Error(char *pszFormat, ...)
    {
    DWORD dwError;
    va_list pArgs;
    LPWSTR lpwMsgBuf;
    char *lpMsgBuf;
    int n, m;
    size_t l;

    dwError = GetLastError();
    va_start(pArgs, pszFormat);
    n = vprintf(pszFormat, pArgs);
    l = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		       FORMAT_MESSAGE_FROM_SYSTEM |
		       FORMAT_MESSAGE_IGNORE_INSERTS,
		       NULL,
		       dwError,
		       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		       (LPWSTR)&lpwMsgBuf, // Address of the allocated buffer address
		       0,
		       NULL
		      );
    l = 2*(l+1); /* Worst case increase for UTF16 to UTF8 conversion */
    lpMsgBuf = (char *)LocalAlloc(LPTR, l);
    if (lpMsgBuf) {
      m = WideCharToMultiByte(CP_UTF8, 0, lpwMsgBuf, -1, lpMsgBuf, (int)l, NULL, NULL);
      n += printf("Error %08lX: %s\n", dwError, lpMsgBuf);
      LocalFree(lpMsgBuf);
    }
    LocalFree(lpwMsgBuf);

    return n;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsSwitch						      |
|									      |
|   Description:    Test if a command line argument is a switch.	      |
|									      |
|   Parameters:     char *pszArg					      |
|									      |
|   Returns:	    TRUE or FALSE					      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1997/03/04 JFL Created this routine				      |
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
|   Function:	    MyAnsiToUtf8      					      |
|									      |
|   Description:    Convert a string from the ANSI characters to UTF-8.	      |
|									      |
|   Parameters:     char *pszString	ANSI string 			      |
|									      |
|   Returns:	    0 = FAILED; Non 0 = Done.      			      |
|									      |
|   Notes:	    There's a AnsiToOem macro that does almost the same thing.|
|									      |
|   History:								      |
|    1998-04-10 JFL Created routine MyAnsiToOem				      |
|    2015-01-15 JFL Recycled it as MyAnsiToUtf8, to be eventually removed.    |
*									      *
\*---------------------------------------------------------------------------*/

int ConvertCodePage(char *pszString, int iOldCp, int iNewCp)
    {
    size_t len;
    PWSTR pwszName;
    int iDone;

    len = strlen(pszString);
    // oemprintf("Initial string %s\n", pszString);
    pwszName = (PWSTR)malloc(2*(len+1));
    iDone = MultiByteToWideChar(iOldCp, MB_PRECOMPOSED, pszString, (int)len, pwszName, (int)len+1);
    if (!iDone) PrintWin32Error("Failed to convert \"%s\" to wide chars.\n", pszString);
    iDone = WideCharToMultiByte(iNewCp, 0, pwszName, (int)len, pszString, (int)len+1, NULL, NULL);
    if (!iDone) PrintWin32Error("Failed to convert \"%s\" back to normal chars.\n", pszString);
    // oemprintf("Converted string %s\n", pszString);
    free(pwszName);

    return iDone;
    }

int MyAnsiToUtf8(char *pszString)
    {
    return ConvertCodePage(pszString, CP_ACP, CP_UTF8);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    WaitForAnyKey					      |
|									      |
|   Description:    Wait for a key before continuing			      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    None						      |
|									      |
|   Notes:	    Flushes any pending input.				      |
|									      |
|   History:								      |
|									      |
|    1998/06/15 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void _cdecl WaitForAnyKey(void)
    {
    while (kbhit()) getch();	// Flush pending input
    oemprintf("Press any key to exit ");
    while ( !kbhit() ) ;	// Wait for the next input
    getch();			// Flush it
    oemprintf("\n");
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    oemprintf						      |
|									      |
|   Description:    printf a line, converting it to the OEM charset first.    |
|									      |
|   Parameters:     Same as printf					      |
|									      |
|   Returns:	    Same as printf					      |
|									      |
|   Notes:	    Output limited to 1024 characters.			      |
|									      |
|   History:								      |
|									      |
|    1998/06/15 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl oemprintf(const char *pszFormat, ...)
    {
    char szLine[1024];
    va_list pArgs;
    int n;

    va_start(pArgs, pszFormat);
    n = vsprintf(szLine, pszFormat, pArgs);
    MyAnsiToUtf8(szLine);
    n = printf("%s", szLine);
    return n;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    InstallNtDriver					      |
|									      |
|   Description:    Install an NT driver in the registry                      |
|									      |
|   Parameters:     char *pszServiceName    Name used to access the service   |
|		    char *pszFileName	    Device driver pathname	      |
|		    char *pszFriendlyName   User-friendly name to display     |
|		    int iStartType	    When the service is to start      |
|									      |
|   Returns:	    TRUE if installed, FALSE if not.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/03/27 JFL Adapted from Helian Faure's version for Kiss kernel.      |
*									      *
\*---------------------------------------------------------------------------*/

BOOL InstallNtDriver(char *pszServiceName, char *pszFileName,
		     char *pszFriendlyName, int iStartType)
    {
    SC_HANDLE  schSCManager;
    SC_HANDLE  schService;
    BOOL       bRet = FALSE;	// Assume failure

    // printf("Installing %s as driver %s\n", pszFileName, pszServiceName);

    // Open the Service Manager
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) return bRet; // Failure

    // Creates the entries for the standalone driver.
    schService = CreateService (schSCManager,          // SCManager database
				pszServiceName,        // name of service
				pszFriendlyName,       // name to display
				SERVICE_ALL_ACCESS,    // desired access
				SERVICE_KERNEL_DRIVER, // service type
				iStartType,	       // start type
				SERVICE_ERROR_NORMAL,  // error control type
				pszFileName,           // service's binary
				NULL,                  // no load ordering group
				NULL,                  // no tag identifier
				NULL,                  // no dependencies
				NULL,                  // LocalSystem account
				NULL                   // no password
				);
    if (schService) // If succeeded to install the service
	{
	bRet = TRUE;	    // Flag success
	CloseServiceHandle(schService);
	}
    else  // Else failed to install the service
	{ // But there is one case where this is not really an error:
	if (GetLastError() == ERROR_SERVICE_EXISTS)
	    bRet = TRUE;    // Flag success
	}

    CloseServiceHandle(schSCManager);
    return bRet;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    RemoveNtDriver					      |
|									      |
|   Description:    Uninstall an NT driver from the registry                  |
|									      |
|   Parameters:     char *pszServiceName    Name used to access the service   |
|									      |
|   Returns:	    TRUE if uninstalled, FALSE if not.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/03/27 JFL Adapted from Helian Faure's version for Kiss kernel.      |
*									      *
\*---------------------------------------------------------------------------*/

BOOL RemoveNtDriver(char *pszServiceName)
    {
    SC_HANDLE  schSCManager;
    SC_HANDLE  schService;
    BOOL       bRet = FALSE;	// Assume failure

    // Open the Service Manager
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) return bRet; // Failure

    // Open the requested service
    schService = OpenService(schSCManager, pszServiceName, SERVICE_ALL_ACCESS);
    if (schService)
        {
	bRet = DeleteService(schService);
        CloseServiceHandle(schService);
        }

    CloseServiceHandle (schSCManager);
    return bRet;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    StartNtDriver					      |
|									      |
|   Description:    Start an NT driver			                      |
|									      |
|   Parameters:     char *pszServiceName    Name used to access the service   |
|									      |
|   Returns:	    TRUE if started, FALSE if not.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/03/27 JFL Adapted from Helian Faure's version for Kiss kernel.      |
*									      *
\*---------------------------------------------------------------------------*/

BOOL StartNtDriver(char *pszServiceName)
    {
    SC_HANDLE  schSCManager;
    SC_HANDLE  schService;
    BOOL       bRet = FALSE;	// Assume failure

    // Open the Service Manager
    schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if (!schSCManager) return bRet; // Failure

    // Open the requested service
    schService = OpenService(schSCManager, pszServiceName, SERVICE_ALL_ACCESS);
    if (schService)
	{
	bRet = StartService(schService, 0, NULL);
        if (!bRet)  // Failed to start the service
	    {	    // But there is one case where this is not really an error:
            if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
		{
		bRet = TRUE;
		}
	    }
        CloseServiceHandle (schService);
        }

    CloseServiceHandle (schSCManager);
    return bRet;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    StopNtDriver					      |
|									      |
|   Description:    Stop an NT driver			                      |
|									      |
|   Parameters:     char *pszServiceName    Name used to access the service   |
|									      |
|   Returns:	    TRUE if stopped, FALSE if not.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/03/27 JFL Adapted from Helian Faure's version for Kiss kernel.      |
*									      *
\*---------------------------------------------------------------------------*/

BOOL StopNtDriver(char *pszServiceName)
    {
    SC_HANDLE  schSCManager;
    SC_HANDLE  schService;
    SERVICE_STATUS  serviceStatus;
    BOOL       bRet = FALSE;	// Assume failure

    // Open the Service Manager
    schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if (!schSCManager) return bRet; // Failure

    // Open the requested service
    schService = OpenService(schSCManager, pszServiceName, SERVICE_ALL_ACCESS);
    if (schService)
	{
        bRet = ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
        CloseServiceHandle (schService);
	}

    CloseServiceHandle (schSCManager);
    return bRet;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ListNtDrivers					      |
|									      |
|   Description:    Display the list of loaded NT drivers                     |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    TRUE if done, FALSE if failed.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1998-10-06 JFL Initial implementation.				      |
|    2005-12-19 JFL Display error messages in case of failure.		      |
|    2014-12-17 JFL Bug fix: List failed if the buffer size requirement       |
|                   increased between the first call and the second.	      |
*									      *
\*---------------------------------------------------------------------------*/

/* Visual Studio 6 does not define these */
#ifndef _In_
#define _In_
#endif
#ifndef _Out_
#define _Out_
#endif
#ifndef _Out_opt_
#define _Out_opt_
#endif
#ifndef _Inout_opt_
#define _Inout_opt_
#endif

BOOL WINAPI EnumServicesStatusU( /* UTF-8 version of EnumServicesStatus() */
  _In_         SC_HANDLE hSCManager,
  _In_         DWORD dwServiceType,
  _In_         DWORD dwServiceState,
  _Out_opt_    LPENUM_SERVICE_STATUS lpServices,
  _In_         DWORD cbBufSize,
  _Out_        LPDWORD pcbBytesNeeded,
  _Out_        LPDWORD lpServicesReturned,
  _Inout_opt_  LPDWORD lpResumeHandle
) {
  BOOL bRet;
  DWORD dw;
  char buf[1024];
  bRet = EnumServicesStatusW(hSCManager,
			     dwServiceType,
			     dwServiceState,
			     (LPENUM_SERVICE_STATUSW)lpServices,
			     cbBufSize,
			     pcbBytesNeeded,
			     lpServicesReturned,
			     lpResumeHandle);
  /* If we were just requesting a size, then return it immediately */
  if ((!lpServices) && (!cbBufSize)) return bRet;
  /* Else convert the Unicode names to UTF-8 */
  if (bRet) for (dw=0; dw<*lpServicesReturned; dw++) {
    int n;

    n = Convert2Utf8(lpServices[dw].lpServiceName, buf, sizeof(buf));
    if (!n) PrintWin32Error("Failed to convert service name to UTF-8.\n");

    n = Convert2Utf8(lpServices[dw].lpDisplayName, buf, sizeof(buf));
    if (!n) PrintWin32Error("Failed to convert display name to UTF-8.\n");
  }
  return bRet;
}

BOOL ListNtDrivers(DWORD dwType, DWORD dwState)
    {
    SC_HANDLE	hSCManager;
    BOOL	bRet = FALSE;	// Assume failure
    ENUM_SERVICE_STATUS *lpServiceStatus;
    DWORD	dwBytesNeeded;
    DWORD	dwNumber;
    DWORD	hResume;
    DWORD	dw;
    DWORD	dwErr;

    if (!dwType) dwType = SERVICE_DRIVER;
    if (!dwState) dwState = SERVICE_ACTIVE;

    // Open the Service Manager
    hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ENUMERATE_SERVICE);
    if (!hSCManager)
	{
	PrintWin32Error("Failed to open the service manager.\n");
	return bRet; // Failure
	}

    // Get the amount of memory needed
    hResume = 0;
    bRet = EnumServicesStatusU(
		hSCManager,		// handle to service control manager database
		dwType,		 	// type of services to enumerate
		dwState,	 	// state of services to enumerate
		NULL,			// pointer to service status buffer
		0,			// size of service status buffer
		&dwBytesNeeded,		// pointer to variable for bytes needed
		&dwNumber,		// pointer to variable for number returned
		&hResume);		// pointer to variable for next entry
    if (bRet || (GetLastError() != ERROR_MORE_DATA))
	{
	CloseServiceHandle (hSCManager);
	PrintWin32Error("Failed to get driver list size.\n");
	return FALSE;
	}
    DEBUG_PRINTF(("EnumServicesStatus() requires a %ld bytes buffer\n", (unsigned long)dwBytesNeeded));
    DEBUG_PRINTF(("  for %ld bytes ENUM_SERVICE_STATUS structures.\n", (unsigned long)sizeof(ENUM_SERVICE_STATUS)));

    lpServiceStatus = (LPENUM_SERVICE_STATUS) LocalAlloc(LPTR, dwBytesNeeded);
    if (!lpServiceStatus)
	{
	CloseServiceHandle (hSCManager);
	return FALSE;
	}

    // List all services
    hResume = 0;
    do { /* 2014-12-17 JFL Even though we were told above that a certain buffer size
      	    was needed, be ready to loop for more, as we sometimes see cases where
      	    the number of drivers apparently increased between the 2 calls */
	DEBUG_PRINTF(("EnumServicesStatus(..., dwNumber=%ld, ...)\n", (unsigned long)dwNumber));

	dwNumber = 0;
	bRet = EnumServicesStatusU(
		    hSCManager,		// handle to service control manager database
		    dwType,		// type of services to enumerate
		    dwState,	 	// state of services to enumerate
		    lpServiceStatus,	// pointer to service status buffer
		    dwBytesNeeded,		// size of service status buffer
		    &dwBytesNeeded,		// pointer to variable for bytes needed
		    &dwNumber,		// pointer to variable for number returned
		    &hResume);		// pointer to variable for next entry
	DEBUG_PRINTF(("EnumServicesStatus() returned %d\n", bRet));
	dwErr = 0;
	if (!bRet) dwErr = GetLastError();
	if (bRet || (dwErr == ERROR_MORE_DATA)) for (dw=0; dw<dwNumber; dw++)
	    {
	    DEBUG_PRINTF(("NtDriverStatus(%s) // %s\n", lpServiceStatus[dw].lpServiceName, lpServiceStatus[dw].lpDisplayName));
	    NtDriverStatus(lpServiceStatus[dw].lpServiceName);
	    }
	else
	    {
	    PrintWin32Error("Failed to enumerate drivers. Needed=%ld\n", dwBytesNeeded);
	    break;
	    }
    } while (dwErr == ERROR_MORE_DATA);

    LocalFree(lpServiceStatus);
    CloseServiceHandle (hSCManager);
    return bRet;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    NtDriverStatus					      |
|									      |
|   Description:    Display the status of a given NT driver                   |
|									      |
|   Parameters:     char *pszServiceName    Name used to access the service   |
|									      |
|   Returns:	    TRUE if done, FALSE if failed.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/10/06 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

SC_HANDLE WINAPI OpenServiceU(
  _In_  SC_HANDLE hSCManager,		// Handle to service control manager database
  _In_  LPCTSTR lpServiceName,		// Pointer to name of service to open
  _In_  DWORD dwDesiredAccess		// Type of access to service
) {
  WCHAR wszName[256];
  int n;

  n = MultiByteToWideChar(CP_UTF8, 0, lpServiceName, -1, wszName, sizeof(wszName)/sizeof(WCHAR));
  if (!n) {
    PrintWin32Error("Failed to convert service name to UTF-8.\n");
    return 0;
  }
  return OpenServiceW(hSCManager, wszName, dwDesiredAccess);
}

BOOL WINAPI QueryServiceConfigU(
  _In_       SC_HANDLE hService,
  _Out_opt_  LPQUERY_SERVICE_CONFIG lpServiceConfig,
  _In_       DWORD cbBufSize,
  _Out_      LPDWORD pcbBytesNeeded
) {
  BOOL bRet;
  char buf[65536];
  size_t n;

  bRet = QueryServiceConfigW(
		hService,					// Handle to service
		(LPQUERY_SERVICE_CONFIGW)lpServiceConfig,	// Address of service config. structure
		cbBufSize,					// Size of service configuration buffer
		pcbBytesNeeded);				// aAddress of variable for bytes needed);
  if (!lpServiceConfig && !cbBufSize) {
    return bRet;
  }

  n = Convert2Utf8(lpServiceConfig->lpBinaryPathName, buf, sizeof(buf));
  if (!n) PrintWin32Error("Failed to convert lpBinaryPathName to UTF-8.\n");

  n = Convert2Utf8(lpServiceConfig->lpLoadOrderGroup, buf, sizeof(buf));
  if (!n) PrintWin32Error("Failed to convert lpLoadOrderGroup to UTF-8.\n");

  n = Convert2Utf8(lpServiceConfig->lpDependencies, buf, sizeof(buf));
  if (!n) PrintWin32Error("Failed to convert lpDependencies to UTF-8.\n");

  n = Convert2Utf8(lpServiceConfig->lpServiceStartName, buf, sizeof(buf));
  if (!n) PrintWin32Error("Failed to convert lpServiceStartName to UTF-8.\n");

  n = Convert2Utf8(lpServiceConfig->lpDisplayName, buf, sizeof(buf));
  if (!n) PrintWin32Error("Failed to convert lpDisplayName to UTF-8.\n");

  return bRet;
}

BOOL NtDriverStatus(char *pszServiceName) /* pszServiceName = UTF-8 name */
    {
    BOOL	bRet;
    SC_HANDLE	hSCManager;
    SC_HANDLE	hService;
    SERVICE_STATUS sServiceStatus;
    LPQUERY_SERVICE_CONFIG lpServiceConfig;
    DWORD	dwBytesNeeded;
    char	*psz;

    // Open the Service Manager
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (!hSCManager) return FALSE; // Failure

    // Open the service
    printf("%-22s", pszServiceName);
    hService = OpenServiceU(
		hSCManager,		// handle to service control manager database
		pszServiceName,		// pointer to name of service to start
		GENERIC_READ);		// type of access to service
    if (!hService)
	{
	printf(" Unknown\n", pszServiceName); /* Align this with other services states */
	CloseServiceHandle (hSCManager);
	return FALSE;
	}

    // Get the service status
    psz = "Unknown";
    bRet = QueryServiceStatus(
		hService,		// handle of service
		&sServiceStatus);	// Address of service status structure
    if (bRet)
	{
	switch (sServiceStatus.dwCurrentState)
	    {
	    case SERVICE_STOPPED: psz = "Stopped"; break;
	    case SERVICE_START_PENDING: psz = "Starting"; break;
	    case SERVICE_STOP_PENDING: psz = "Stopping"; break;
	    case SERVICE_RUNNING: psz = "Started"; break;
	    case SERVICE_CONTINUE_PENDING: psz = "Continuing"; break;
	    case SERVICE_PAUSE_PENDING: psz = "Pausing"; break;
	    case SERVICE_PAUSED: psz = "Paused"; break;
	    default: psz = "Unknown"; break;
	    }
	}
    printf(" %-10s", psz);

    bRet = QueryServiceConfigU(
		hService,		// handle of service
		NULL,			// address of service config. structure
		0,			// size of service configuration buffer
		&dwBytesNeeded);	// address of variable for bytes needed);
    if (bRet || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
	{
	CloseServiceHandle(hSCManager);
	PrintWin32Error("Failed to get the service configuration.\n");
	return FALSE;
	}
    lpServiceConfig = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, dwBytesNeeded);
    if (!lpServiceConfig)
	{
	CloseServiceHandle(hSCManager);
	printf("Error: Not enough memory to get the service configuration.\n");
	return FALSE;
	}
    bRet = QueryServiceConfigU(
		hService,		// handle of service
		lpServiceConfig,			// address of service config. structure
		dwBytesNeeded,			// size of service configuration buffer
		&dwBytesNeeded);	// address of variable for bytes needed);
    if (bRet)
	{
	switch (lpServiceConfig->dwStartType)
	    {
	    case SERVICE_BOOT_START: psz = "Boot"; break;
	    case SERVICE_SYSTEM_START: psz = "System"; break;
	    case SERVICE_AUTO_START: psz = "Auto"; break;
	    case SERVICE_DEMAND_START: psz = "Demand"; break;
	    case SERVICE_DISABLED: psz = "Disabled"; break;
	    default: psz = "Unknown"; break;
	    }
	printf(" %-9s", psz);
	printf(" %s", lpServiceConfig->lpDisplayName);
	if (iVerbose)
	    {
	    printf("\n%10sCommand=%s", "", lpServiceConfig->lpBinaryPathName);
	    printf("\n%10sGroup=%s", "", lpServiceConfig->lpLoadOrderGroup);
	    printf("\n%10sAccount=%s", "", lpServiceConfig->lpServiceStartName);
	    }
	}

    printf("\n");
    CloseServiceHandle (hSCManager);
    return bRet;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    InstallVxd						      |
|									      |
|   Description:    Install a Win9x VxD in the know VxD list in the registry  |
|									      |
|   Parameters:     char *pszServiceName    Name used to access the service   |
|		    char *pszFileName	    Device driver pathname	      |
|									      |
|   Returns:	    TRUE if installed, FALSE if not.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/10/02 JFL Extracted from the main routine.			      |
*									      *
\*---------------------------------------------------------------------------*/

BOOL InstallVxd(char *pszServiceName, char *pszFileName)
    {
    HKEY hKey;
    char szCanonicName[MAX_PATH];
    char szCorrectedName[MAX_PATH];
    LPSTR lpNodeName;
    LONG lError;
    DWORD dwLen;

    // Get the canonical pathname of the VxD.
    GetFullPathName(pszFileName, // pointer to name of file to find path for
		    sizeof(szCanonicName), // size, in characters, of path buffer
		    szCanonicName, // pointer to path buffer
		    &lpNodeName); // pointer to filename in path
    if (iVerbose) oemprintf("Installing %s as driver %s\n", szCanonicName, pszServiceName);

    // ~~jfl 1998/04/10 Added workaround for Win95/98 bug:
    // Win95 cannot start a VxD located in a directory with an é in its name,
    //	using the CreateFile() function. Note: This works using the 16-bits
    //	int 2F interface to VPOWERD.VxD :-).
    // A workaround for the bug is to find the full path for the VxD, and to
    //	convert it first into the OEM character set.
    dwLen = GetShortPathName(szCanonicName, szCorrectedName, sizeof(szCorrectedName));
    if (!dwLen)
	{
	PrintWin32Error("Failed to get the short pathname.\n");
	return FALSE;
	}
    if (iVerbose) printf("The equivalent short name is %s\n", szCorrectedName);

    // Create an entry in the known VxDs list
    lError = RegCreateKey(HKEY_LOCAL_MACHINE,
	"System\\CurrentControlSet\\Control\\SessionManager\\KnownVxDs",
	&hKey);
    if (lError != ERROR_SUCCESS)
	{
	PrintWin32Error("Failed to open the registry.\n");
	return FALSE;
	}

    lError = RegSetValueEx(hKey, // handle of key to set value for
	pszServiceName, // address of value to set
	0, // reserved
	REG_SZ, // flag for value type
	(CONST BYTE *)szCorrectedName, // address of value data
	lstrlen(szCorrectedName) + 1); // size of value data
    if (lError != ERROR_SUCCESS)
	{
	PrintWin32Error("Failed to write to the registry.\n");
	RegCloseKey(hKey);
	return FALSE;
	}

    RegCloseKey(hKey);
    return TRUE;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LoadVxd						      |
|									      |
|   Description:    Load a VxD in memory				      |
|									      |
|   Parameters:     char *pszServiceName    Name used to access the service   |
|									      |
|   Returns:	    TRUE if started, FALSE if not.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/10/02 JFL Extracted from the main routine.			      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE LoadVxd(char *pszServiceName)
    {
    char szDriverName[MAX_PATH] = "\\\\.\\";
    HANDLE hDriver;

    /* First attempt to load the device or file name as it is */
    oemprintf("Loading %s\n", pszServiceName);
    hDriver = CreateFile(pszServiceName,
		    GENERIC_READ | GENERIC_WRITE,
		    FILE_SHARE_READ,
		    NULL,
		    OPEN_EXISTING,
		    0,	// Do not use FILE_FLAG_DELETE_ON_CLOSE as it would not stay loaded after this program exits.
		    NULL);
    if (iVerbose) oemprintf("Returned handle %08X.\n", hDriver);
    if (hDriver != (HANDLE)INVALID_HANDLE_VALUE) return hDriver;

    // There's nothing else we can do if this is a Win32 device name.
    if (!strncmp(pszServiceName, "\\\\.\\", 4)) return NULL;

    /* Else try to convert a naked device name to a Win32 device name */
    lstrcpy(szDriverName+4, pszServiceName);
    return LoadVxd(szDriverName);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    UnloadVxd	  					      |
|									      |
|   Description:    Unload a VxD from memory		                      |
|									      |
|   Parameters:     char *pszDeviceName	    Name used to access the VxD       |
|									      |
|   Returns:	    TRUE if stopped, FALSE if not.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/04/24 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

BOOL UnloadVxd(char *pszDeviceName)
    {
    char szDriverName[MAX_PATH] = "\\\\.\\";
    BOOL bDone;

    bDone = DeleteFile(pszDeviceName);
    if (bDone) return TRUE;

    // There's nothing else we can do if this is a Win32 device name.
    if (!strncmp(pszDeviceName, "\\\\.\\", 4)) return FALSE;

    /* Else try to convert a naked device name to a Win32 device name */
    lstrcpy(szDriverName+4, pszDeviceName);
    return UnloadVxd(szDriverName);
    }

#if 0

/* VXDLDR definitions */

enum			// Win32 DeviceIoControl codes
    {
    VXDLDR_GETVERSION,
    VXDLDR_LOAD,
    VXDLDR_UNLOAD,
    VXDLDR_DEVINITSUCCEEDED,
    VXDLDR_DEVINITFAILED,
    VXDLDR_GETDEVICELIST,
    };

struct ObjectInfo {
    ULONG OI_LinearAddress;	// start addr of object
    ULONG OI_Size;		// size of object in bytes
    ULONG OI_ObjType;		// obj type, defined in loader.h
    ULONG OI_Resident;		// Static object ?
    };

struct DeviceInfo {
    struct DeviceInfo *DI_Next;
    UCHAR DI_LoadCount; 	    // Reference Count
    struct VxD_Desc_Block *DI_DDB;  // pointer to DDB
    USHORT DI_DeviceID;
    CHAR *DI_ModuleName;	    // module name as stored in LE header
    ULONG DI_Signature; 	    // signature to verify the struc
    ULONG DI_ObjCount;		    // number of objects
    struct ObjectInfo *DI_ObjInfo;  // pointer to array of ObjectInfos
    ULONG DI_V86_API_CSIP;	    // Save area for v86 api entry point
    ULONG DI_PM_API_CSIP;	    // Save area for pm api entry point
    };

BOOL UnloadVxd(char *pszDeviceName)
    {
    HANDLE hVxdLoader;
    DWORD nReturned;
    struct
	{
	WORD wVxdLdrError;
	WORD wDeviceId;
	char szName[80];
	} sBuf;

    hVxdLoader = CreateFile("\\\\.\\VXDLDR",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
		    0,
                    NULL);

    if (hVxdLoader == (HANDLE)INVALID_HANDLE_VALUE)
	{
	PrintWin32Error("Cannot open VXDLDR.VxD.\n");
        return FALSE;
	}

    sBuf.wDeviceId = 0xFFFF;
    lstrcpy(sBuf.szName, pszDeviceName);
    do	{
	DeviceIoControl(hVxdLoader, VXDLDR_UNLOAD,
			&sBuf, sizeof(sBuf),	    // input buffer
			&sBuf, sizeof(sBuf),	    // Output buffer
			&nReturned, NULL);
	}
	while (!sBuf.wVxdLdrError);

    CloseHandle(hVxdLoader);

    return TRUE;
    }
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    RemoveVxd						      |
|									      |
|   Description:    Remove a Win9x VxD in the know VxD list in the registry   |
|									      |
|   Parameters:     char *pszServiceName    Name used to access the service   |
|									      |
|   Returns:	    TRUE if removed, FALSE if not.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/10/02 JFL Extracted from the main routine.			      |
*									      *
\*---------------------------------------------------------------------------*/

BOOL RemoveVxd(char *pszServiceName)
    {
    HKEY hKey;
    LONG lError;

    lError = RegCreateKey(HKEY_LOCAL_MACHINE,
	"System\\CurrentControlSet\\Control\\SessionManager\\KnownVxDs",
	&hKey);
    if (lError != ERROR_SUCCESS)
	{
	PrintWin32Error("Failed to open the registry.\n");
	return FALSE;
	}

    lError = RegDeleteValue(hKey, pszServiceName);
    if (lError != ERROR_SUCCESS)
	{
	PrintWin32Error("Failed to write to the registry.\n");
	RegCloseKey(hKey);
	return FALSE;
	}

    RegCloseKey(hKey);

    return TRUE;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetVxdldrVersion					      |
|									      |
|   Description:    Display VXDLDR.VxD version				      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    TRUE if done, FALSE if failed.			      |
|									      |
|   Notes:	    THIS ROUTINE DOES NOT WORK. 			      |
|		    Actually it's VXDLDR.VxD that does not.                   |
|									      |
|   History:								      |
|									      |
|    1998/10/07 JFL Extracted from the main routine.			      |
*									      *
\*---------------------------------------------------------------------------*/

enum			// Win32 DeviceIoControl codes
    {
    VXDLDR_GETVERSION,
    VXDLDR_LOAD,
    VXDLDR_UNLOAD,
    VXDLDR_DEVINITSUCCEEDED,
    VXDLDR_DEVINITFAILED,
    VXDLDR_GETDEVICELIST,
    };

#define NW 1

BOOL GetVxdldrVersion(void)
    {
    HANDLE hVxdLoader;
    DWORD nReturned;
    DWORD dwVersion[NW];
    int i;

    hVxdLoader = CreateFile("\\\\.\\VXDLDR",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
		    0,
                    NULL);

    if (hVxdLoader == (HANDLE)INVALID_HANDLE_VALUE)
	{
	PrintWin32Error("Cannot open VXDLDR.VxD.\n");
        return FALSE;
	}

    for (i=0; i<NW; i++) dwVersion[i] = 0;
    if (DeviceIoControl(hVxdLoader, VXDLDR_GETVERSION,
			NULL, 0,			    // Input buffer
			&dwVersion[0], sizeof(dwVersion),   // Output buffer
			&nReturned, NULL))
	{
	if (nReturned >= 4)
	    printf("VXDLDR version %08lx", dwVersion[0]);
	else if (nReturned >= 2)
	    printf("VXDLDR version %04x", WORD0(dwVersion[0]));
	else
	    printf("No version returned");
	for (i=2; i<=NW; i++) if (nReturned >= (4UL*i))
	    printf(" %08lx", dwVersion[i-1]);
	printf(".\n");
	}
    else
	{
	PrintWin32Error("No version returned\n");
	}

    CloseHandle(hVxdLoader);

    return TRUE;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    ListVxds						      |
|									      |
|   Description:    Enumerate VXDs					      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    TRUE if done, FALSE if failed.			      |
|									      |
|   Notes:	    							      |
|		    							      |
|   History:								      |
|    2015-01-15 JFL Created this routine.           			      |
*									      *
\*---------------------------------------------------------------------------*/

#if 0

BOOL ListVxds(void) {
  fprintf(stderr, "Error: VxD enumeration not implemented yet\n");
  return FALSE;
}

#endif

/*
From http://www.woodmann.com/forum/archive/index.php/t-2634.html:
VxDCall VXDLDR_GetDeviceList is used for dynamically loaded vxds.
VMMCall VMM_GetVxDLocationList is used for statically loaded vxds.
With these 2 calls, you can get a list of all the static and dynamically loaded vxds on your system.
This is how Softice generates the list you see when you type 'vxd'.

Ring3 to Ring0 switch code available at:
http://vxheaven.org/lib/vzo26.html

From http://www.woodmann.com/forum/archive/index.php/t-3238.html:
VXDLDR_GetDeviceList returns a pointer to a linked list of DeviceInfo structures (See VXDLDR definitions above),
each one containing a pointer to the VxD DDB.
*/

#if 1

BOOL ListVxds(void) {
    HANDLE hVxdLoader;
    DWORD nReturned;
    void *pDeviceList;

    hVxdLoader = CreateFile("\\\\.\\VXDLDR",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
		    0,
                    NULL);

    if (hVxdLoader == (HANDLE)INVALID_HANDLE_VALUE)
	{
	PrintWin32Error("Cannot open VXDLDR.VxD.\n");
        return FALSE;
	}

    if (DeviceIoControl(hVxdLoader, VXDLDR_GETDEVICELIST,
			NULL, 0,			    // Input buffer
			&pDeviceList, sizeof(pDeviceList),  // Output buffer
			&nReturned, NULL))
	{
	if (nReturned >= 4)
	    printf("pDeviceList = %08lx\n", pDeviceList);
	else
	    printf("Nothing returned\n");
	}
    else
	{
	PrintWin32Error("No pointer returned\n");
	}

    CloseHandle(hVxdLoader);

    return TRUE;
}

#endif

#if 0

/*
Based on sample code at
http://www.vijaymukhi.com/vmis/vmchap8.htm

Does not work: Triggers a protection fault
*/

#pragma pack(1)
struct DDB
{
	unsigned long next;
	unsigned short ver;
	unsigned short no;
	unsigned char Mver;
	unsigned char minorver;
	unsigned short flags;
	unsigned char name[8];
	unsigned long Init;
	unsigned long ControlProc;
	unsigned long v86_proc;
	unsigned long Pm_Proc;
	void (*v86)();
	void (*PM)();
	unsigned long data;
	unsigned long service_size;
	unsigned long win32_ptr;
};

BOOL ListVxds(void) {
  struct DDB *v;
  char *s = 0xC0001000L;
  char n[9];
  char aa[100];

  while (strncmp(s,"VMM    ",8)) s++;
  v = (struct DDB *)(s-12);
  printf("Name            Number  Size\n");
  while(v) {
    strncpy(n,v->name,8);
    n[8]='\0';
    printf("%-15s %6d  %ld\n", n, v->no, v->service_size);
    v = (struct DDB *)(v->next);
  }
  return TRUE;
}
#pragma pack()

#endif /* Experimental versions of VxD enumeration */

