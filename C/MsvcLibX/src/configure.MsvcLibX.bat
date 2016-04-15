:# Special make actions for rebuilding the MsvcLibX library

:# Get the full pathname of the MsvcLibX library base directory
for /f "delims=" %%d in ('"pushd .. & cd & popd"') do SET MSVCLIBX=%%d

:# Set the environment variable in config.h for use in the make file
%CONFIG% set "MSVCLIBX=%MSVCLIBX%"

:# Declare the SDKs and libraries we need
%BEGIN_SDK_DEFS%
%USE_SDK% MSVCLIBX
%END_SDK_DEFS%

:# Set the local environment variable just before make exits, so that future commands in this CMD window have it.
%CONFIG% set POSTMAKEACTION=set "MSVCLIBX=%MSVCLIBX%"

:# Set the system environment variable, so that other CMD windows opened later on inherit it
setx MSVCLIBX %MSVCLIBX% >NUL

