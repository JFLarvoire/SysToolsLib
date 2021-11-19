Python scripts
==============

This directory is a placeholder for future scripts written in Python.

Installation
------------

1. Under Linux  

   All modern Linux distributions already include a Python interpreter.

   - Copy the *.py scripts into a directory in your path. For example: /usr/local/bin
   - Make sure they're executable: chmod +x /usr/local/bin/*.py
   - Remove the .py extensions, so that they can be invoked with just their base name.

2. Under Windows

   Install a Python interpreter

   - Download the latest stable Python interpreter from ActiveState:  
     http://www.activestate.com/activepython/downloads  
   - Install it on your system. Accept defaults for all options, unless you know what you're doing.
    
   Then
    
   - Copy all batch and Python scripts into your PATH. For example into C:\Windows:

         copy *.bat %windir%
         copy *.py %windir%

   - Run `PySetup.bat -s`  
     (This sets up Windows to run .py files as command-line scripts.)
     
   - In Windows XP or 2003, close the command prompt and restart it. Not necessary in more recent versions of Windows.

   Script      | Description
   ----------- | ---------------------------------------------------------------------------------------------------------
   PySetup.bat | Setup Windows to run .py files as command-line scripts.
   python.bat  | Run the python interpreter, even if it's not in the PATH. Allows choosing one instance if there are multiple Python instances installed.
   pip.bat     | Run pip.exe, even if it's not in the PATH. Allows choosing one instance if there are multiple Python instances installed.

   All scripts display a help screen when invoked with the -? option.
