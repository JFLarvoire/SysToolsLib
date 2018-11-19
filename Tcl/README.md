Tcl scripts
===========

Installation
------------

Most of these scripts work both in Windows and in Linux. 

1. Under Linux  

    All modern Linux distributions already include Tcl and Expect interpreters.

    - Copy the scripts into a directory in you path. For example: /usr/local/bin
    - Make sure they're executable: chmod +x /usr/local/bin/*.tcl
    - Remove the .tcl extensions, so that they can be invoked with just their base name.
    - Some scripts depend on external packages for some of their features.  
      If needed, make sure the following RPMs are installed: tcllib, tcltls  
      If they're not, use rpm, yum, apt-get, etc, to install them.  
      Note: On RedHat Enterprise Linux distributions, the tcltls RPM is not available in the yum repositories.  
            But it's possible to use the tcltls RPM from the equivalent Fedora distribution instead.

2. Under Windows

    1. Install a Tcl interpreter

        - Download the latest stable 32-bits Tcl interpreter from ActiveState:  
          http://www.activestate.com/activetcl/downloads  
          Be sure to download the x86 version, even on AMD64/x86_64 machines, because we use Expect,
          and it's only available in the x86 version.
        - Install it on your system. Accept defaults for all options, unless you know what you're doing.
        - Copy [tclsh.bat](../Batch) and [TclSetup.bat](../Batch) into your path. Ex: into C:\Windows
        - Run `TclSetup.bat -s`  
          (This sets up Windows to run .tcl files as command-line scripts, and .tk files as windowed scripts.)
        - In Windows XP or 2003, close the command prompt and restart it. Not necessary in more recent versions of Windows.

    2. Install the optional Tcl packages some scripts need  

        At the command prompt, run: (Using an --http-proxy option (or not) as necessary for your site)

        ```
        teacup install --http-proxy web-proxy:8080 Expect
        teacup install --http-proxy web-proxy:8080 dns
        teacup install --http-proxy web-proxy:8080 twapi
        ```  

        Note that many scripts are able to automatically download the packages they need.  
        For the remaining cases where this is not automated, use commands similar to the above two to get the missing packages.

