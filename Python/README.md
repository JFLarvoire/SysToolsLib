Python scripts
==============

This directory is a placeholder for future scripts written in Python.

Installation
------------

1. Under Linux  

    All modern Linux distributions already include a Python interpreter.

    - Copy the scripts into a directory in your path. For example: /usr/local/bin
    - Make sure they're executable: chmod +x /usr/local/bin/*.py
    - Remove the .py extensions, so that they can be invoked with just their base name.

2. Under Windows

    Install a Python interpreter

    - Download the latest stable Python interpreter from ActiveState:  
      http://www.activestate.com/activepython/downloads  
    - Install it on your system. Accept defaults for all options, unless you know what you're doing.
    - Copy [PySetup.bat](PySetup.bat) into your path. Ex: into C:\Windows
    - Run `PySetup.bat -s`  
      (This sets up Windows to run .py files as command-line scripts.)
    - In Windows XP or 2003, close the command prompt and restart it. Not necessary in more recent versions of Windows.
