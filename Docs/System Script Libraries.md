# System Script Libraries for Tcl/Tk, Bash, Batch, PowerShell

The System Tools Library includes four system script libraries, for the four scripting languages that I regularly use.
These libraries regroup functions that I found useful over the years, for developing and debugging powerful system management scripts.
Every time I reused them, I made small improvements. After 25 years of experience, and many redesigns later, they embody what I think are the best principles and methods for building good scripts in these languages. (More on this further down.)

Have a look and give them a try. They can help you write powerful new scripts... or debug difficult problems in scripts you already have.

Note that these libraries are (and hopefully will always be) a work in progress. Some versions lag behind others. Some inconsistencies remain that should be cleaned up. I'll be happy to get feedback, improvement ideas, and maybe even contributions!

Here's a short summary. For a full description, download the detailed file [System Script Libraries Description.htm](System Script Libraries Description.htm) in SysToolsLib's Docs area.

## Library structure

The 4 core modules share a common structure:

* Each library is called Library.xxx, and is located in its language-specific directory.
* The core of each library, at the beginning of each module, is a script debugging framework. (Different for each language, but based on common principles.)
* It is followed by an eclectic list of system management routines. The list varies a lot depending on language.
* Finally there's a main routine, processing a set of standard command line options. (The same for all languages. Use option -? to display a help screen.)

## Usage

Each library can be used in different ways:

* As a template for creating new scripts with powerful self-debugging abilities.
* For Tcl/Tk, and eventually for the other languages, as a standalone module that can be sourced in to add debugging capabilities to other scripts.
* As a repository of routines, to pick and choose for inclusion in other scripts.
* As a standalone script for testing the library itself.

## Principles and Features

The common goals and design principles are:

* Scripts should have built-in debugging capabilities.
  They should not rely on outside debuggers, which are not always available in the field, or too complex to use by end-users. (In the case of Windows' batch language, there is simply no debugger available at all!)
* Scripts should not have distinct "normal" and "debug" versions.
  They should work "normally" by default, and have standard options for enabling special modes:
   * The verbose mode, enabled by the -v option, displays more details than normal. This is to help users understand what is being done.
   * The debug mode, enabled by the -d option, displays intermediate variables values, traces code execution, etc. This is to help the script author fix bugs.
   * The no-execute mode, aka. WhatIf mode in PowerShell, enabled by the -X option, displays all "dangerous" commands that should run, without actually running them. This is to help cautious people (both the author and the users) forecast catastrophes that may or may not happen if you run the script "normally".
* The debugging libraries should be as lightweight as possible.
   * This allows to leave them in permanently, with no performance penalty when debugging is disabled.
   * Adding them to an outside script should require as few changes as possible. (Ideally just one: Inserting the library in the beginning of the script! In reality, more or less depending on languages.)
   * Library functions should replace and extend existing language functions if possible. Else use a name almost identical to the library function it replaces. (To minimize changes, and if changes are required to minimize the apparent change (and memorization effort.). Ex: echo becomes Echo.
   * Using advanced features in the library should be very easy. For example, in all languages, enabling the no-exec mode for a dangerous command is as easy as inserting a single keyword ahead of the command line. (Such as Exec)
* The debugging libraries should use each language built-in debugging capabilities as much as possible.
   * No need to reinvent the wheel.
   * Being incompatible with the standard debugging functions would make it more difficult to retrofit our library into existing scripts.
* The debugging output should be reusable as input in other shell instances. Even the progress and status messages should be valid comments.
  This is a very important capability:
   * This allows to manually single-step through problematic operations, by copying the debug output from a window where the failure occurred, and pasting it into another shell window.
   * This also allows, in combination with the -X (no-exec) mode, to generate fully functional scripts as output. This is very useful to explain to others what the script does in a particular case. This is also very useful to quickly generate a modified script for a particular case not managed by the original script.

A consequence of this is that the debug output syntax is language-specific. What is common across implementations is the principles to achieve that.

* Function call/return tracing is indented by call depth. In debug mode, echoed strings are indented too.
  This makes is considerably easier to browse the debug output of complex scripts with deep call trees.
   * For example this is much easier to read that bash -x output.
   * As far as I know, the ability to do that for Windows' Batch scripts is unique in the world.
* There must be an easy way to log all output; Possibly by default; And without having to explicitly request it for every command.
  If logging is enabled, the whole debug output goes to the log. Even when not in debug mode.
