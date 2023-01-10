/*****************************************************************************\
*                                                                             *
*   File name	    ShareInfo.c						      *
*		    							      *
*   Description	    Get information about a shared folder on a remote server  *
*		    							      *
*   Notes	    References:						      *
*		    https://docs.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-a-remote-computer
*		    https://superuser.com/questions/465038/finding-the-physical-local-path-associated-with-a-share-unc-folder
*		    							      *
*		    TODO: There is often a long timeout in ConnectServer().   *
*		    	  Yet this does not happen when using wmic.exe.	      *
*		    	  What is done differently in wmic.exe?	Ex:	      *
*		          wmic /node:SERVER share where name="SHARE" get path *
*		    							      *
*   History								      *
*    2021-11-23 JFL Created this file, based on cpuid.c.		      *
*    2021-11-25 JFL Added the optional \\SERVER\SHARE argument syntax.	      *
*		    Added the ability to enumerate all properties.	      *
*    2021-11-26 JFL Display the error facility.                       	      *
*		    							      *
*         © Copyright 2021 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Get information about a shared folder on a remote server"
#define PROGRAM_NAME    "ShareInfo"
#define PROGRAM_VERSION "2021-11-26"

/* Definitions */

#ifndef _WIN32
#error "This uses WMI APIs only available in Windows"
#endif

#pragma warning(disable:4001)       /* Ignore the // C++ comment warning */

#include "predefine.h"

#define _WIN32_DCOM

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <iconv.h>
/* SysToolsLib include files */
#include "debugm.h"
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

#define streq(s1, s2) (!strcmp(s1, s2))     /* Test if strings are equal */

/* Forward references */

void usage(void);
int GetWmiShareInfo(char *pszServer, char *pszShare, char *pszProp, void *lpBuf, size_t lBuf);
int GetWmiShareInfos(char *pszServer, char *pszShare, char **ppszProps, void *lpBuf, size_t lBuf);

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description	    EXE program main initialization routine		      |
|									      |
|   Parameters	    int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The return code to pass to the BIOS, if run from ROM.     |
|									      |
|   History								      |
|    2021-11-23 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int _cdecl main(int argc, char *argv[]) {
  int i;
  int iVerbose = FALSE;
  char *pszServer = NULL;
  char *pszShare = NULL;
  char *pszProp = NULL;
  char buf[4096];
  int iRet;
  char **ppszProps = NULL;	// NULL-terminated list of properties to display
  int nProps = 0;		// Number of properties in the above list
  char *pValue;

  /* Process arguments */

  for (i=1 ; i<argc ; i++) {
    char *arg = argv[i];
    if ((arg[0] == '-') || (arg[0] == '/')) {  /* It's a switch */
      char *opt = arg+1;
      if (streq(opt, "?")) {		/* -?: Help */
	usage();                            /* Display help */
      }
      if (streq(opt, "d")) {		/* -d: Debug information */
	DEBUG_ON();
	continue;
      }
      if (streq(opt, "v")) {		/* -v: Verbose information */
	iVerbose = TRUE;
	continue;
      }
      if (streq(opt, "V")) {		/* -V: Display version information */
	puts(DETAILED_VERSION);
	return 0;
      }
      fprintf(stderr, "Error: Unexpected option: %s\n", arg);
      return 1;
    }
    if (!pszServer) { /* Optionally allow the \\SERVER\SHARE syntax */
      if ((arg[0] == '\\') && (arg[1] == '\\')) arg += 2;
      pszServer = arg;
      pszShare = strchr(pszServer, '\\');
      if (pszShare) *(pszShare++) = '\0';
      continue;
    }
    if (!pszShare) {
      pszShare = arg;
      continue;
    }
    // Any other argument is a property name
    ppszProps = realloc(ppszProps, (nProps + 2) * sizeof(char *));
    if (!ppszProps) {
      fprintf(stderr, "Error: Not enough memory\n", arg);
      return 1;
    }
    ppszProps[nProps++] = arg;
    ppszProps[nProps] = NULL;
    continue;
  }

  if (nProps == 1) {	// Test the 1-property version
    iRet = GetWmiShareInfo(pszServer, pszShare, ppszProps[0], buf, (int)sizeof(buf));
  } else {		// Test the general version
    iRet = GetWmiShareInfos(pszServer, pszShare, ppszProps, buf, (int)sizeof(buf));
  }
  if (iRet == -1) {
    HRESULT hr = *(HRESULT *)buf;
    LPTSTR pszErrMsg = NULL;
    if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0)) {
      DWORD dwError = HRESULT_CODE(hr); /* The HRESULT is convertible to a WIN32 error */
      if (FormatMessage(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	dwError,
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
	(LPTSTR)&pszErrMsg,
	0,
	NULL )) {
	int l = lstrlen(pszErrMsg);
	/* Remove the trailing new line and dot, if any. */
	if (l && (pszErrMsg[l-1] == '\n')) pszErrMsg[--l] = '\0';
	if (l && (pszErrMsg[l-1] == '\r')) pszErrMsg[--l] = '\0';
	if (l && (pszErrMsg[l-1] == '.')) pszErrMsg[--l] = '\0';
	fprintf(stderr, "Error: %s\n", pszErrMsg);
	LocalFree(pszErrMsg); /* Free the buffer allocated by FormatMessage() */
      }
    }
    if (!pszErrMsg) { // Report the HRESULT if we did not find a WIN32 equivalent
      // See https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/0642cb2f-2075-4469-918c-4441e69c548a?redirectedfrom=MSDN
      char *pszFacility = "";
      switch (((ULONG)hr >> 16) & 0x87FF) { // Extract the facility number
      	// A selection of the likely facilities that may fail us
	case 0x8001: pszFacility = "RPC "; break;
	case 0x8002: pszFacility = "COM dispatch "; break;
	case 0x8003: pszFacility = "OLE storage "; break;
	case 0x8004: pszFacility = "WBEM "; break;		// The general case is "COM/OLE Interface ", but for us this means WBEM
	case 0x8008: pszFacility = "Security "; break;
	case 0x8009: pszFacility = "Security API "; break;
	case 0x800B: pszFacility = "Certificate "; break;
	case 0x8011: pszFacility = "COM+ "; break;
	case 0x8033: pszFacility = "WinRM "; break;
	case 0x8050: pszFacility = "Defender "; break;
	default: pszFacility = ""; break;
      }
      fprintf(stderr, "%sError: HRESULT 0x%X\n", pszFacility, hr); 
    }
    return 1;
  } else if (iRet == 0) {
    fprintf(stderr, "Error: No data found for share %s\n", pszShare);
    return 1;
  } else {
    pValue = buf;
    for (i=0; i < iRet; i++) {
      char szProp[32] = "";
      char *pszQuote = "";
      VARTYPE vt;

      if (ppszProps) {	// We provided the property names
      	pszProp = ppszProps[i];
      	if (!pszProp) break;
      } else {		// The function returned the property names
      	pszProp = pValue;
      	pValue += lstrlen(pValue) + 1;
      }
      if (!ppszProps || ppszProps[1]) { // If there are several properties to display, prepend their name
      	sprintf(szProp, "%s ", pszProp);
      	pszQuote = "\"";  // And put quotes around strings
      }

      vt = *(VARTYPE*)pValue;
      pValue += sizeof(VARTYPE);
      switch (vt) {
      case VT_NULL: // There is no value
	if (szProp[0]) printf("%s\n", pszProp); // Print just the name, or nothing at all
	break;
      case VT_BOOL:
	printf("%s%s\n", szProp, *(BOOL *)pValue ? "TRUE" : "FALSE");
	pValue += 2;
	break;
      case VT_BSTR:
	printf("%s%s%s%s\n", szProp, pszQuote, pValue, pszQuote);
	pValue += lstrlen(pValue) + 1;
	break;
      case VT_I1:
	printf("%s%d\n", szProp, *(CHAR *)pValue & 0xFF);
	pValue += 1;
	break;
      case VT_UI1:
	printf("%s%u\n", szProp, *(UCHAR *)pValue & 0xFFU);
	pValue += 1;
	break;
      case VT_I2:
	printf("%s%d\n", szProp, *(SHORT *)pValue & 0xFFFF);
	pValue += 2;
	break;
      case VT_UI2:
	printf("%s%u\n", szProp, *(USHORT *)pValue & 0xFFFFU);
	pValue += 2;
	break;
      case VT_I4:
      case VT_INT:
	printf("%s%ld\n", szProp, *(LONG *)pValue & 0xFFFFFFFFL);
	pValue += 4;
	break;
      case VT_UI4:
      case VT_UINT:
	printf("%s%lu\n", szProp, *(ULONG *)pValue & 0xFFFFFFFFLU);
	pValue += 4;
	break;
      default: /* See enum VARENUM definitions in wtypes.h */
	fprintf(stderr, "Error: %sVARTYPE %u is not supported\n", szProp, vt);
	break;
      }
    }
  }

  return 0;
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
|    2021-11-23 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void usage(void) {
  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n\
Usage: " PROGRAM_NAME " [switches] SERVER SHARE [PROPERTY ...]\n\
   or: " PROGRAM_NAME " [switches] \\\\SERVER\\SHARE [PROPERTY ...]\n\
\n\
Optional switches:\n\
\n\
  -?    Display this help screen and exit.\n"
#ifdef _DEBUG
"\
  -d    Output debug information\n"
#endif
"\
  -V    Display this program version and exit\n\
\n\
Server:   Server name, or . for the local machine\n\
Share:    Share name\n\
Property: Any valid share property name. Default: Display common properties\n\
"
#include "footnote.h"
);
  exit(0);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetWmiShareInfo					      |
|									      |
|   Description	    Get shared folder information from WMI		      |
|									      |
|   Parameters	    							      |
|									      |
|   Returns	    -1: Error; Else if >= 0: VARTYPE of the result	      |
|									      |
|   Notes	    Based on samples in					      |
|                   https://docs.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-the-local-computer
|		    https://www.codeproject.com/Articles/10539/Making-WMI-Queries-In-C
|                   https://stackoverflow.com/questions/1431103/how-to-obtain-data-from-wmi-using-a-c-application
|		    https://stackoverflow.com/questions/18992717/list-all-properties-of-wmi-class-in-c
|                                                                             |
|		    WBEM API doc here:                                        |
|		    https://docs.microsoft.com/en-us/windows/win32/api/wbemcli/nn-wbemcli-iwbemclassobject
|		    SAFEARRAY doc here:                                       |
|		    https://docs.microsoft.com/en-us/windows/win32/api/oaidl/ns-oaidl-safearray?redirectedfrom=MSDN
|                                                                             |
|   History	    							      |
|    2021-11-23 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32

// Note: #define _WIN32_DCOM before including windows.h above

#include <wbemidl.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wbemuuid.lib")

int GetWmiShareInfo(char *pszServer, char *pszShare, char *pszProp, void *lpBuf, size_t lBuf) {
  char *ppszProps[2];
  ppszProps[0] = pszProp;
  ppszProps[1] = NULL;
  return GetWmiShareInfos(pszServer, pszShare, ppszProps, lpBuf, lBuf);
}

int GetWmiShareInfos(char *pszServer, char *pszShare, char **ppszProps, void *lpBuf, size_t lBuf) {
  HRESULT hr = E_INVALIDARG;
  int nResult = -1;
  char *pszProp;
  IWbemLocator         *pLoc  = NULL;
  IWbemServices        *pSvc = NULL;
  IEnumWbemClassObject *pEnum = NULL;
  WCHAR *pwszServer = NULL;
  WCHAR *pwszShare = NULL;
  WCHAR *pwszProp = NULL;
  WCHAR *pwszBuf = NULL;
  // BSTR strings we'll use (http://msdn.microsoft.com/en-us/library/ms221069.aspx)
  BSTR resource = NULL;
  BSTR language = NULL;
  BSTR query    = NULL;
  WCHAR wszResource[256];
  int iGetNames = (ppszProps == NULL);
  char **ppsz;

  DEBUG_ENTER(("GetWmiShareInfos(\"%s\", \"%s\", %p, %p, %u);\n", pszServer, pszShare, ppszProps, lpBuf, lBuf));

  if (!pszServer || !pszShare || !lpBuf || !lBuf) goto fail_and_exit;

  hr = E_OUTOFMEMORY;

  pwszServer = NewWideCopy(pszServer);
  pwszShare = NewWideCopy(pszShare);
  if (!pwszServer || !pwszShare) goto fail_and_exit;

  wsprintfW(wszResource, L"\\\\%s\\ROOT\\CIMV2", pwszServer );

  resource = SysAllocString(wszResource);
  language = SysAllocString(L"WQL");

  pwszBuf = (LPWSTR)malloc(sizeof(WCHAR)*(lstrlen(pszShare) + 40));
  if (pwszBuf) {
    wsprintfW(pwszBuf, L"SELECT * FROM Win32_Share WHERE Name='%s'", pwszShare);
    query = SysAllocString(pwszBuf);
  }

  if (!resource || !language || !query) goto fail_and_exit;

  // 1. Initialize COM
  DEBUG_PRINTF(("CoInitializeEx()\n"));
  hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hr)) goto fail_and_exit;

  // 2. Set general COM security levels
  DEBUG_PRINTF(("CoInitializeSecurity()\n"));
  // Authentication levels: ag RPC_C_AUTHN "C:\Program Files (x86)\Microsoft Visual Studio 8\VC\PlatformSDK\Include"
  // Impersonation levels: ag RPC_C_IMP_ "C:\Program Files (x86)\Microsoft Visual Studio 8\VC\PlatformSDK\Include"
  // Capabilities flags: ag EOAC_ "C:\Program Files (x86)\Microsoft Visual Studio 8\VC\PlatformSDK\Include"
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL); // => OK, but long connect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_ANONYMOUS, NULL, EOAC_NONE, NULL); // => Query() fails. HRESULT 0x80041003 = WBEM_E_ACCESS_DENIED
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_DEFAULT, NULL, EOAC_NONE, NULL); // => CoInitializeSecurity() fails. The parameter is incorrect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_ANONYMOUS, NULL, EOAC_NONE, NULL); // => Query() fails. HRESULT 0x80041003 again
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_DELEGATE, NULL, EOAC_NONE, NULL); // => Connect() fails. A security package specific error occurred
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_DEFAULT, NULL, EOAC_NONE, NULL); // => CoInitializeSecurity() fails. The parameter is incorrect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL); // => OK, but long connect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_ANONYMOUS, NULL, EOAC_NONE, NULL); // => Query() fails. HRESULT 0x80041003 again
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL); // => OK, but long connect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DEFAULT, NULL); // => CoInitializeSecurity() fails. The parameter is incorrect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_AUTO_IMPERSONATE, NULL); // => CoInitializeSecurity() fails. The parameter is incorrect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DISABLE_AAA, NULL); // => OK, but long connect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_ANY_AUTHORITY, NULL); // => CoInitializeSecurity() fails. The parameter is incorrect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_MAKE_FULLSIC, NULL); // => OK, but long connect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL); // => OK, but long connect
//  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, NULL); // => Query() fails. HRESULT 0x80041003 = WBEM_E_ACCESS_DENIED
  hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
  if (FAILED(hr)) goto fail_and_exit;

  // 3. Obtain the initial locator to WMI
  DEBUG_PRINTF(("CoCreateInstance()\n"));
  hr = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *) &pLoc);
  if (FAILED(hr)) goto fail_and_exit;

  // 4. Connect to WMI
  DEBUG_PRINTF(("ConnectServer()\n"));
  hr = pLoc->lpVtbl->ConnectServer(pLoc, resource, NULL, NULL, NULL, WBEM_FLAG_CONNECT_USE_MAX_WAIT, NULL, NULL, &pSvc);
  if (FAILED(hr)) goto fail_and_exit;

  // 5. Set security levels on the proxy
  // Apparently not needed here

  // 6. Execute the WQL query
  DEBUG_PRINTF(("ExecQuery()\n"));
  hr = pSvc->lpVtbl->ExecQuery(pSvc, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &pEnum);
  if (FAILED(hr)) goto fail_and_exit;

  // 7. Get the data from the query
  DEBUG_PRINTF(("# Get the data\n"));
  if (pEnum) {
    IWbemClassObject *pObj = NULL;
    ULONG nReturn = 0;

    // Enumerate the retrieved objects
    // https://docs.microsoft.com/en-us/windows/win32/api/oaidl/ns-oaidl-variant
    // https://docs.microsoft.com/en-us/windows/win32/api/wtypes/ne-wtypes-varenum
    nResult = 0;
    while((hr = pEnum->lpVtbl->Next(pEnum, WBEM_INFINITE, 1, &pObj, &nReturn)) == S_OK) {
      int iProp;
      char *pData = lpBuf;
      size_t nLeft = lBuf;
      int iSize;

      if (iGetNames) { // No names provided. Query the whole list of properties
      	SAFEARRAY *psaNames;
      	long lLower, lUpper, l;
	BSTR bsPropName = NULL;
      	int nNames;

      	hr = pObj->lpVtbl->GetNames(pObj, NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &psaNames);
	if (FAILED(hr)) goto fail_and_exit;
	SafeArrayGetLBound(psaNames, 1, &lLower);
	SafeArrayGetUBound(psaNames, 1, &lUpper);
	nNames = (int)(lUpper + 1 - lLower);
	DEBUG_PRINTF(("# There are %d properties\n", nNames));
	if (ppszProps) { // Free the previous set, if any, before reallocating a new set
	  for (ppsz = ppszProps; *ppsz; ppsz++) free(*ppsz);
	  free(ppszProps);
	}
	ppszProps = (char **)malloc((nNames+1)*sizeof(char *));
	if (!ppszProps) {
	  SafeArrayDestroy(psaNames);
	  goto fail_and_exit;
	}
        for (iProp = 0, l = lLower; l <= lUpper; l++) { // Get this property.
	  hr = SafeArrayGetElement(psaNames, &l, &bsPropName);
	  ppszProps[iProp++] = NewUtf8Copy(bsPropName);
	  SysFreeString(bsPropName);
        }
        ppszProps[iProp] = NULL;
        SafeArrayDestroy(psaNames);
      }

      memset(lpBuf, 0, lBuf);
      for (iProp = 0; (pszProp = ppszProps[iProp]) != NULL; iProp++) {
	VARIANT vtProp;

	DEBUG_PRINTF(("Get(%s)\n", pszProp));
	if (iGetNames) { // Return the property name
	  iSize = lstrlen(pszProp) + 1;
	  if (nLeft <= (size_t)iSize) {
	    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	    goto fail_and_exit;
	  }
	  strcpy(pData, pszProp);
	  pData += iSize;
	  nLeft -= iSize;
	}
	free(pwszProp); // Free the previous one, if any
	pwszProp = NewWideCopy(pszProp);
	if (!pwszProp) {
	  hr = E_OUTOFMEMORY;
	  goto fail_and_exit;
	}

	hr = pObj->lpVtbl->Get(pObj, pwszProp, 0, &vtProp, 0, 0);
	if (FAILED(hr)) goto fail_and_exit;
	nResult += 1;

	DEBUG_PRINTF(("# It's VARTYPE %u\n", vtProp.vt));

	// Copy the result to the caller's buffer
	if (nLeft > sizeof(VARTYPE)) {
	  *(VARTYPE *)pData = (VARTYPE)(vtProp.vt);
	  pData += sizeof(VARTYPE);
	  nLeft -= sizeof(VARTYPE);
	}
	iSize = -1;
	switch (vtProp.vt) {
	case VT_NULL:
	  iSize = 0;
	  break;
	case VT_I1:
	case VT_UI1:
	  if (nLeft >= 1) {
	    *(CHAR *)pData = vtProp.cVal;
	    iSize = 1;
	  }
	  break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
	  if (nLeft >= 2) {
	    *(USHORT *)pData = vtProp.uiVal;
	    iSize = 2;
	  }
	  break;
	case VT_I4:
	case VT_UI4:
	case VT_INT:
	case VT_UINT:
	  if (nLeft >= 4) {
	    *(ULONG *)pData = vtProp.ulVal;
	    iSize = 4;
	  }
	  break;
	case VT_BSTR:
	  iSize = WideCharToMultiByte(CP_UTF8, 0, vtProp.bstrVal, -1, pData, (int)nLeft, NULL, NULL);
	  if (iSize == 0) { // The function failed, most likely because the buffer was too small
	    nResult = -1;
	    hr = HRESULT_FROM_WIN32(GetLastError());
	  }
	  break;
	default:
	  nResult = -1;
	  hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	  fprintf(stderr, "Unsupported VARIANT type: %d\n", vtProp.vt);
	}
	if ((nResult != -1) && (iSize == -1)) {
	  nResult = -1;
	  hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}
	if (nResult == -1) break;
	pData += iSize;
	nLeft -= iSize;
      }

      // Release the current result object
      pObj->lpVtbl->Release(pObj);

      if (nResult == -1) break;
    }
  }

  goto cleanup_and_exit;

fail_and_exit:
  nResult = -1;

cleanup_and_exit:
  DEBUG_PRINTF(("# Cleanup\n"));
  // Release WMI COM interfaces
  if (pEnum) pEnum->lpVtbl->Release(pEnum);
  if (pSvc) pSvc->lpVtbl->Release(pSvc);
  if (pLoc) pLoc->lpVtbl->Release(pLoc);
  CoUninitialize();

  // Free everything else we've allocated
  free(pwszServer);
  free(pwszShare);
  free(pwszProp);
  free(pwszBuf);
  SysFreeString(query);
  SysFreeString(language);
  SysFreeString(resource);

  if (iGetNames && ppszProps) { // Free the last set of names
    for (ppsz = ppszProps; *ppsz; ppsz++) free(*ppsz);
    free(ppszProps);
  }

  if ((nResult < 0) && (lBuf >= sizeof(HRESULT))) *(HRESULT *)lpBuf = hr;
  RETURN_INT_COMMENT(nResult, ((nResult > 0) ? "Success\n" : "Failure\n"));
}

#endif // defined(_WIN32)
