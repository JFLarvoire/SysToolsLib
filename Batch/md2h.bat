@echo off & rem { /* CSS comment hiding the batch section from CSS
:##############################################################################
:#		    							      #
:#  Filename        md2h.bat                                                  #
:#		    							      #
:#  Description     Convert GitHub MarkDown to HTML, and display it           #
:#		    							      #
:#  Notes 	    This file is a dual Batch+CSS script.		      #
:#		    Dummy CSS definitions on the first line hide the batch    #
:#		    section inside a CSS comment.			      #
:#		    							      #
:#		    Uses the GitHub markdown API:			      #
:#		    https://developer.github.com/v3/markdown/		      #
:#                                                                            #
:#		    Includes the stylesheet (w. its copyright message) from:  #
:#		    https://gist.github.com/tuzz/3331384		      #
:#		    							      #
:#  Author:	JFL jf.larvoire@hpe.com	    Jean-Francois Larvoire	      #
:#		    							      #
:#  History:	    							      #
:#   2019-11-30 JFL Created this script.				      #
:#   2019-12-09 JFL Display which server was used.			      #
:#   2019-12-13 JFL Display an error message if curl failed.		      #
:#   2020-07-03 JFL Added options -c & -t to select the github server to use. #
:#		    Added option -t to use an authentication token.           #
:#		    Added option -s to use a given github server API URL.     #
:#   2023-01-10 JFL Fixed a potential issue with a `for /f` command.          #
:#		                                                              #
:#         © Copyright 2019 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2023-01-10"
set "SCRIPT=%~nx0"		&:# Script name
set "SNAME=%~n0"		&:# Script name, without its extension
set "SPATH=%~dp0"		&:# Script path
set "SPATH=%SPATH:~0,-1%"	&:# Script path, without the trailing \
set "SFULL=%~f0"		&:# Script full pathname
set ^"ARG0=%0^"			&:# Script invokation name
set ^"ARGS=%*^"			&:# Argument line

set "GITHUB.COM=https://api.github.com"	&:# URL of the public GitHub API
set "GITHUB.CORP="			&:# URL of the corporate GitHub API
:# Within HPE, prefer HPE's corporate github server
:# Also this avoids having to configure the http_proxy environment variable.
nslookup github.hpe.com. 2>NUL | findstr /r /c:"[ 	]github.hpe.com$" >NUL
if not errorlevel 1 set "GITHUB.CORP=https://github.hpe.com/api/v3"

goto :main

:#----------------------------------------------------------------------------#

:# Quote file pathnames that require it.
:condquote	 %1=Input variable. %2=Opt. output variable.
setlocal EnableExtensions Disabledelayedexpansion
set "RETVAR=%~2"
if not defined RETVAR set "RETVAR=%~1" &:# By default, change the input variable itself
call set "P=%%%~1%%"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Remove double quotes inside P. (Fails if P is empty)
set "P=%P:"=%"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Look for any special character that needs quoting
:# Added "@" that needs quoting ahead of commands.
:# Added "|&<>" that are not valid in file names, but that do need quoting if used in an argument string.
echo."%P%"|findstr /C:" " /C:"&" /C:"(" /C:")" /C:"[" /C:"]" /C:"{" /C:"}" /C:"^^" /C:"=" /C:";" /C:"!" /C:"'" /C:"+" /C:"," /C:"`" /C:"~" /C:"@" /C:"|" /C:"&" /C:"<" /C:">" >NUL
if not errorlevel 1 set P="%P%"
:condquote_ret
:# Contrary to the general rule, do NOT enclose the set commands below in "quotes",
:# because this interferes with the quoting already added above. This would
:# fail if the quoted string contained an & character.
:# But because of this, do not leave any space around & separators.
endlocal&set %RETVAR%=%P%&exit /b 0

:#----------------------------------------------------------------------------#

:is_newer FILE1 FILE2
:# Query xcopy to know if the copy by date would be done.
:# If the copy is done, the file name is output.
:# If that name does not contain "\", then the drive name "C:" is prepended!
:# So search either for a : or a \
if not exist %2 exit /b 0 &:# ERRORLEVEL 0 if target is missing
xcopy /d /l /y %1 %2 | findstr ": \\" >nul &:# ERRORLEVEL 0 if newer, 1 if older
if errorlevel 1 %ECHO.D% %2 is already up-to-date
exit /b

:#----------------------------------------------------------------------------#

:help
echo %SCRIPT% - Convert GitHub MarkDown to HTML, and display it
echo.
echo Usage: %SCRIPT% [OPTIONS] PATHNAME
echo.
echo Options:
echo   -?       Display this help message and exit
echo   -c       Use the Corporate GitHub (Default: Use it if possible)
echo   -p       Use the Public GitHub (Default: Use it if no alternative)
echo   -s URL   Use the GitHub API at this URL
echo   -t TOKEN Use this authentication token. (See note 2)
echo   -v       Display curl progress messages
echo   -V       Display the script version and exit
echo   -X       Display the commands to execute, but don't run them
echo.
echo Notes:
echo 1) This program uses curl.exe. For old Windows versions that don't have it,
echo    curl.exe can be downloaded from https://curl.haxx.se/windows/
echo 2) By default, uses the authentication token defined in the optional file
echo    "%%USERPROFILE%%\.config\md2h\SERVER.DOMAIN.COM.txt"
exit /b 0

:#----------------------------------------------------------------------------#

:main
set "MD="
set "DEBUG=0"
set "ECHO.D=if 0==1 echo"
set "OPTS= -s"
set "EXEC="
set ">=>"
set ">>=>>"
set "TRUE.EXE=(call,)"	&:# Macro to silently set ERRORLEVEL to 0
set "GITHUB="
set "TOKEN="

goto :get_arg
:next_arg
shift
:get_arg
set "ARG=%~1"
if [%1]==[] goto :start
if "%ARG%"=="-?" goto :help
if "%ARG%"=="/?" goto :help
if "%ARG%"=="-c" set "GITHUB=%GITHUB.CORP%" & goto :next_arg
if "%ARG%"=="-d" set "DEBUG=1" & set "ECHO.D=echo" & goto :next_arg
if "%ARG%"=="-p" set "GITHUB=%GITHUB.COM%" & goto :next_arg
if "%ARG%"=="-s" set "GITHUB=%~2" & shift & goto :next_arg
if "%ARG%"=="-t" set "TOKEN=%~2" & shift & goto :next_arg
if "%ARG%"=="-v" set "OPTS=%OPTS: -s=%" & goto :next_arg
if "%ARG%"=="-V" (echo %VERSION%) & exit /b
if "%ARG%"=="-X" set "EXEC=echo" & set ">=^>" & set ">>=^>^>" & goto :next_arg
if "%ARG:~0,1%"=="-" >&2 echo Error: Unexpected switch: %1 & exit /b 1
if not defined MD set "MD=%~1" & goto :next_arg
>&2 echo Error: Unexpected argument: %1
exit /b 1

:start
if not defined MD goto :help
if not exist "%MD%" (
  >&2 echo Error: File "%MD%" not found
  exit /b 1
)
if not defined GITHUB set "GITHUB=%GITHUB.CORP%"
if not defined GITHUB set "GITHUB=%GITHUB.COM%"
%ECHO.D% # Server: %GITHUB%
for %%f in ("%MD%") do set "HTML=%TEMP%\%%~nf.htm"
%ECHO.D% # HTML file: "%HTML%"
set "CSS=%TEMP%\github.css"
%ECHO.D% # CSS file: "%HTML%"
for %%f in ("%CSS%") do set "HREF=%%~nxf"

setlocal EnableExtensions EnableDelayedExpansion

:# Unfortunately, Chrome refuses to load style sheets from files, unless they end with the .css extension.
:# So do not load this batch directly, but instead make a renamed copy in the %TEMP% directory.
:# Update the .css copy if needed
call :is_newer "!SFULL!" "!CSS!" &:# ERRORLEVEL 0 if newer, 1 if older
if not errorlevel 1 (
  %ECHO.D% # Updating "!CSS!"
  %EXEC% copy /y "!SFULL!" "!CSS!" %>% NUL
)

:# Tell the user which of the possible github servers was used.
for %%m in ("%MD%") do set "NXMD=%%~nxm" &:# Extract the .md file name and extension
for /f "delims=/ tokens=2" %%u in ("%GITHUB%") do set "SERVER=%%u" &:# Extract the server DNS name
call :condquote NXMD
echo # Converting %NXMD% using %SERVER%

:# Load the authentication token from "%USERPROFILE%\.config\md2h\%SERVER%.txt" if present
if not defined TOKEN (
  for %%f in ("%USERPROFILE%\.config\md2h\%SERVER%.txt") do if exist %%f (
    for /f "usebackq delims=" %%t in ("%%~f") do set "TOKEN=%%~t"
  )
)

set "LT=<"
set "GT=>"

 > "!HTML!" echo !LT!html!GT!

>> "!HTML!" echo !LT!head!GT!
>> "!HTML!" echo !LT!meta charset="UTF-8"!GT!
>> "!HTML!" echo !LT!link rel="stylesheet" type="text/css" href="!HREF!"!GT!
>> "!HTML!" echo !LT!/head!GT!

>> "!HTML!" echo !LT!body!GT!
if defined TOKEN set OPTS=%OPTS% -H "Authorization: token %TOKEN%"
%TRUE.EXE% &:# Set ERRORLEVEL to 0. Useful in no-exec mode to avoid triggering error messages below.
%EXEC% curl%OPTS% %GITHUB%/markdown/raw -X "POST" -H "Content-Type: text/plain" --data-binary "@!MD!" %>>% "!HTML!"
if errorlevel 9009 (
  >&2 echo This program uses curl.exe. For old Windows versions that don't have it,
  >&2 echo curl.exe can be downloaded from https://curl.haxx.se/windows/
  exit /b 1
)
if errorlevel 1 ( :# curl sometimes fails silently, so add our own message
  >&2 echo curl.exe failed with error %ERRORLEVEL%. Aborting.
  exit /b
)
>> "!HTML!" echo !LT!/body!GT!

>> "!HTML!" echo !LT!/html!GT!

%EXEC% "!HTML!"

exit /b

:#----------------------------------------------------------------------------#
:# Style sheet from https://gist.github.com/tuzz/3331384 - End of batch section comment */ }
/*
Copyright (c) 2017 Chris Patuzzo
https://twitter.com/chrispatuzzo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

body {
  font-family: Helvetica, arial, sans-serif;
  font-size: 14px;
  line-height: 1.6;
  padding-top: 10px;
  padding-bottom: 10px;
  background-color: white;
  padding: 30px;
  color: #333;
}

body > *:first-child {
  margin-top: 0 !important;
}

body > *:last-child {
  margin-bottom: 0 !important;
}

a {
  color: #4183C4;
  text-decoration: none;
}

a.absent {
  color: #cc0000;
}

a.anchor {
  display: block;
  padding-left: 30px;
  margin-left: -30px;
  cursor: pointer;
  position: absolute;
  top: 0;
  left: 0;
  bottom: 0;
}

h1, h2, h3, h4, h5, h6 {
  margin: 20px 0 10px;
  padding: 0;
  font-weight: bold;
  -webkit-font-smoothing: antialiased;
  cursor: text;
  position: relative;
}

h2:first-child, h1:first-child, h1:first-child + h2, h3:first-child, h4:first-child, h5:first-child, h6:first-child {
  margin-top: 0;
  padding-top: 0;
}

h1:hover a.anchor, h2:hover a.anchor, h3:hover a.anchor, h4:hover a.anchor, h5:hover a.anchor, h6:hover a.anchor {
  text-decoration: none;
}

h1 tt, h1 code {
  font-size: inherit;
}

h2 tt, h2 code {
  font-size: inherit;
}

h3 tt, h3 code {
  font-size: inherit;
}

h4 tt, h4 code {
  font-size: inherit;
}

h5 tt, h5 code {
  font-size: inherit;
}

h6 tt, h6 code {
  font-size: inherit;
}

h1 {
  font-size: 28px;
  color: black;
}

h2 {
  font-size: 24px;
  border-bottom: 1px solid #cccccc;
  color: black;
}

h3 {
  font-size: 18px;
}

h4 {
  font-size: 16px;
}

h5 {
  font-size: 14px;
}

h6 {
  color: #777777;
  font-size: 14px;
}

p, blockquote, ul, ol, dl, li, table, pre {
  margin: 15px 0;
}

hr {
  border: 0 none;
  color: #cccccc;
  height: 4px;
  padding: 0;
}

body > h2:first-child {
  margin-top: 0;
  padding-top: 0;
}

body > h1:first-child {
  margin-top: 0;
  padding-top: 0;
}

body > h1:first-child + h2 {
  margin-top: 0;
  padding-top: 0;
}

body > h3:first-child, body > h4:first-child, body > h5:first-child, body > h6:first-child {
  margin-top: 0;
  padding-top: 0;
}

a:first-child h1, a:first-child h2, a:first-child h3, a:first-child h4, a:first-child h5, a:first-child h6 {
  margin-top: 0;
  padding-top: 0;
}

h1 p, h2 p, h3 p, h4 p, h5 p, h6 p {
  margin-top: 0;
}

li p.first {
  display: inline-block;
}

ul, ol {
  padding-left: 30px;
}

ul :first-child, ol :first-child {
  margin-top: 0;
}

ul :last-child, ol :last-child {
  margin-bottom: 0;
}

dl {
  padding: 0;
}

dl dt {
  font-size: 14px;
  font-weight: bold;
  font-style: italic;
  padding: 0;
  margin: 15px 0 5px;
}

dl dt:first-child {
  padding: 0;
}

dl dt > :first-child {
  margin-top: 0;
}

dl dt > :last-child {
  margin-bottom: 0;
}

dl dd {
  margin: 0 0 15px;
  padding: 0 15px;
}

dl dd > :first-child {
  margin-top: 0;
}

dl dd > :last-child {
  margin-bottom: 0;
}

blockquote {
  border-left: 4px solid #dddddd;
  padding: 0 15px;
  color: #777777;
}

blockquote > :first-child {
  margin-top: 0;
}

blockquote > :last-child {
  margin-bottom: 0;
}

table {
  padding: 0;
}
table tr {
  border-top: 1px solid #cccccc;
  background-color: white;
  margin: 0;
  padding: 0;
}

table tr:nth-child(2n) {
  background-color: #f8f8f8;
}

table tr th {
  font-weight: bold;
  border: 1px solid #cccccc;
  text-align: left;
  margin: 0;
  padding: 6px 13px;
}

table tr td {
  border: 1px solid #cccccc;
  text-align: left;
  margin: 0;
  padding: 6px 13px;
}

table tr th :first-child, table tr td :first-child {
  margin-top: 0;
}

table tr th :last-child, table tr td :last-child {
  margin-bottom: 0;
}

img {
  max-width: 100%;
}

span.frame {
  display: block;
  overflow: hidden;
}

span.frame > span {
  border: 1px solid #dddddd;
  display: block;
  float: left;
  overflow: hidden;
  margin: 13px 0 0;
  padding: 7px;
  width: auto;
}

span.frame span img {
  display: block;
  float: left;
}

span.frame span span {
  clear: both;
  color: #333333;
  display: block;
  padding: 5px 0 0;
}

span.align-center {
  display: block;
  overflow: hidden;
  clear: both;
}

span.align-center > span {
  display: block;
  overflow: hidden;
  margin: 13px auto 0;
  text-align: center;
}

span.align-center span img {
  margin: 0 auto;
  text-align: center;
}

span.align-right {
  display: block;
  overflow: hidden;
  clear: both;
}

span.align-right > span {
  display: block;
  overflow: hidden;
  margin: 13px 0 0;
  text-align: right;
}

span.align-right span img {
  margin: 0;
  text-align: right;
}

span.float-left {
  display: block;
  margin-right: 13px;
  overflow: hidden;
  float: left;
}

span.float-left span {
  margin: 13px 0 0;
}

span.float-right {
  display: block;
  margin-left: 13px;
  overflow: hidden;
  float: right;
}

span.float-right > span {
  display: block;
  overflow: hidden;
  margin: 13px auto 0;
  text-align: right;
}

code, tt {
  margin: 0 2px;
  padding: 0 5px;
  white-space: nowrap;
  border: 1px solid #eaeaea;
  background-color: #f8f8f8;
  border-radius: 3px;
}

pre code {
  margin: 0;
  padding: 0;
  white-space: pre;
  border: none;
  background: transparent;
}

.highlight pre {
  background-color: #f8f8f8;
  border: 1px solid #cccccc;
  font-size: 13px;
  line-height: 19px;
  overflow: auto;
  padding: 6px 10px;
  border-radius: 3px;
}

pre {
  background-color: #f8f8f8;
  border: 1px solid #cccccc;
  font-size: 13px;
  line-height: 19px;
  overflow: auto;
  padding: 6px 10px;
  border-radius: 3px;
}

pre code, pre tt {
  background-color: transparent;
  border: none;
}
