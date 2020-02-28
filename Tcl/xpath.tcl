#!/usr/bin/tclsh
#-----------------------------------------------------------------------------#
#                                                                             #
#   File name	    xpath                                                     #
#                                                                             #
#   Description     XML file manager using XPath                              #
#                                                                             #
#   Notes:	    Considers the XML file as a miniature file system.        #
#                   XML nodes are directories.                                #
#                   Node attributes are text files.                           #
#                                                                             #
#                   Options are named after corresponding Unix commands for   #
#                   managing files.                                           #
#                                                                             #
#                   Designed to be used as part of Unix scripts with pipes.   #
#                                                                             #
#   License         Copyright (c) 2006-2013, Jean-François Larvoire	      #
#		    All rights reserved.                                      #
#                                                                             #
#		    Redistribution and use in source and binary forms, with   #
#		    or without modification, are permitted provided that the  #
#		    following conditions are met:                             #
#                                                                             #
#		    * Redistributions of source code must retain the above    #
#		       copyright notice, this list of conditions and the      #
#		       following disclaimer.                                  #
#		    * Redistributions in binary form must reproduce the above #
#		       copyright notice, this list of conditions and the      #
#		       following disclaimer in the documentation and/or other #
#		       materials provided with the distribution.              #
#		                                                              #
#		    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND    #
#		    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED           #
#		    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED    #
#		    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           #
#		    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE  #
#		    COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,#
#		    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL#
#		    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF    #
#		    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR       #
#		    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON  #
#		    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT      #
#		    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    #
#		    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN  #
#		    IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.             #
#		                                                              #
#   Author          Jean-François Larvoire, jf.larvoire@free.fr               #
#                                                                             #
#   History:								      #
#    2006-01-23 JFL Created this script.                                      #
#    2013-09-18 JFL Changed from TclDOM to tDOM, due to TclDOM failure on     #
#                   some machines.                                            #
#    2013-09-25 JFL Added BSD-style license in the header.                    #
#                                                                             #
#-----------------------------------------------------------------------------#
 
set version "2013-09-25"

package require tdom

# Global variables
set script [file tail $argv0]
set path ""             ; # XPath of the XML node to select
set inputFile ""        ; # File to read. Default: stdin.
set hInput ""           ; # File handle
set outputFile ""       ; # File to write. Default: stdout.
set hOutput ""          ; # File handle
set action ""           ; # Action. One of cat, ls, cp, rm...
set smlfs 0		; # 1 = The input comes from the show.tcl -S script

# Debug output control
catch { # Identify my own verbose variable with the dom::tcl one.
  upvar #0 ::dom::verbosity verbose
}
set verbose 1		; # Verbosity. # 0=Quiet 1=Normal 2=Verbose 3=Debug.
proc DebugString {args} {
  if {$::verbose > 2} {eval puts $args}
}
proc VerboseString {args} {
  if {$::verbose > 1} {eval puts $args}
}
proc PutString {args} {
  if {$::verbose > 0} {eval puts $args}
}

# Dump node tree
# node = DOM node to start from
# refResult = {#N name} . Refers to the result string at level #N to append to.
# indent = Indentation level
proc DumpNode {node refResult indent} {
  DebugString "DumpNode $node $refResult $indent ;# level = [info level]"
  set nodeName [$node nodeName]
  set nodeType [$node nodeType]

  eval upvar $refResult result

  set line [format "%*s%s" $indent "" $nodeType]
  switch $nodeType {
    ELEMENT_NODE {
      lappend result "$line $nodeName"
      incr indent 2
      foreach child [$node childNodes] {
	DumpNode $child $refResult $indent
      }
      incr indent -2
    }
    COMMENT_NODE -
    TEXT_NODE {
      set nodeValue [$node nodeValue]
      set textLen [string length $nodeValue]
      lappend result "$line \[$textLen\]"
    }      
    default {
      lappend result "$line $nodeName"
    }
  }
}

# Remove an argument from the head of an argument list.
proc PopArg {{argv ::argv} {argc ::argc}} {
  set arg [lindex [uplevel 1 set $argv] 0] ; # Extract the 1st element.
  uplevel 1 [subst -nocommands {
    set $argv [lrange \$$argv 1 end]       ; # Remove the 1st element.
    if {\$$argc > 0} {incr $argc -1}       ; # Decrease the element count.
  }]
  return $arg
}

# Usage string
set usage {XML file manager using XPath

Usage: $script [OPTIONS] COMMAND XPATH

Options:
  -h | --help           This help message
  -i | --input FILE     Input file name. Default or -: stdin
  -o | --output FILE    Output file name. Default or -: stdout
  -S | --smlfs          The input comes from the show.tcl -S script
  -v | --verbose        Increase the verbosity level

Command:
  cat | type            Display the text content, or the attribute values
  find                  Display the canonic XPath
  ls | dir              List elements and attributes names
  rm | del | rmdir | rd Remove nodes or attributes
  tree                  Display the tree of elements
  xml | .               Display the selected nodes as XML

XPath: Examples: /cluster/node[2]   //node/@name   //node[@name="atlas2"]/@ip
       For full details, see: http://www.w3.org/TR/xpath.
}
#  set {value}           Set the value of a node (XML) or attribute (string)

# Parse the command line
while {$argc > 0} {
  set arg [PopArg]
  # Also added a Windows-friendly alias to each command.
  switch -- $arg {
    "-d" - "--debug" {
      incr verbose 2
    }
    "-h" - "--help" - "-?" - "/?" {
      puts $usage
      exit 0
    }
    "-i" - "--input" {
      set inputFile [PopArg]
    }
    "-o" - "--output" {
      set outputFile [PopArg]
    }
    "-q" - "--quiet" {
      incr verbose -1
    }
    "-S" - "--smlfs" {
      set smlfs 1
    }
    "-v" - "--verbose" {
      incr verbose
    }
    "-V" - "--version" {
      puts $version
      exit 0
    }
    default {
      if {"$action" == ""} {
        set action $arg
      } elseif {"$path" == ""} {
	set path $arg
      } else {
	puts stderr "Unexpected argument: $arg."
	puts $usage
	exit 1
      }
    }
  }
}

# Make sure we got the necessary information
if {"$path" == ""} {
  if {[string index $action 0] == "/"} {
    # Assume we've been given just the XPATH, and we want to see the XML selected
    set path $action
    set action "xml"
  } else {
    # By default, select the entire tree.
    set path "/"
  }
}

# When searching for an attribute, reformat the request
set attrib ""
set list [split $path /]
set last [lindex $list end]
if {"[string index $last 0]" == "@"} { # If the last item is an attribute name,
  set attrib [string range $last 1 end]	 ; # then record its name,
  set list [lrange $list 0 end-1]	 ; # and rebuild the path without it.
  set path [join $list /]
  if {("$action" != "set") && ("$action" != "touch")} { # These can create it if needed.
    append path "\[@$attrib\]" ; # Add a constraint that the attribute must exist.
  }
}

# When searching in an smlfs tree, reformat the request to search for name attributes
if $smlfs {
  set parts ""
  foreach name [split $path /] {
    if {"$name" != ""} {
      set name "*\[@name=\"$name\"\]"
    }
    lappend parts $name
  }
  set path [join $parts "/"]
}

# Display what we will seek
VerboseString "$script: XPath=\"$path\""
if {"$attrib" != ""} {
  VerboseString "$script: AttributeName=\"$attrib\""
}

# Open I/O files
if {("$inputFile" != "") && ("$inputFile" != "-")} {
  set hInput [open $inputFile]
} else {
  set hInput stdin
}
if {("$outputFile" != "") && ("$outputFile" != "-")} {
  set hOutput [open $outputFile w]
} else {
  set hOutput stdout
}

# Parse the XML input
set xml [read $hInput]

set doc [dom parse $xml]
DebugString "$script: XML parsing done. Selecting nodes from $doc."
set root [$doc documentElement]
DebugString "$script: root $root"
set nodes [$root selectNodes $path]
DebugString "$script: nodes $nodes"

# Build the output string
set result ""
switch $action {
  cat - type {  # Display the text in the nodes found, or the requested attribute values
    if {"$attrib" == ""} { # We want whole objects
      foreach node $nodes {
	set innerXML ""
	foreach child [$node childNodes] {
	  if {[$child nodeType] == "TEXT_NODE"} {
	    append innerXML [$child asXML]
	  }
	}
	lappend result $innerXML
      }
      set result [join $result \n]
    } else {               # We want just an attribute per object
      foreach node $nodes {
	lappend result [$node getAttribute $attrib] ; # Build list.
      }
    }
  }
  "cp" - "copy" {	# cp - Copy node or attribute
    set targetPath [PopArg]
    puts "Not implemented yet"
    exit 1
  }
  "find" { # Display the canonic XPath of the nodes found
    foreach node $nodes {
      set pathname [$node toXPath]
      if {"$attrib" != ""} { # If we specified an attribute for that object
	append pathname /@$attrib
      }
      lappend result $pathname
    }
    set result [join $result \n]
  }
  "ls" - "dir" {   # Display the list of attributes for the nodes found
    if {"$path" == "/"} {
      lappend result "[$root nodeName]/"
    } else {
      foreach node $nodes {
	foreach child [$node childNodes] {
	  if {[$child nodeType] == "ELEMENT_NODE"} {
	    lappend result "[$child nodeName]/"
	  }
	}
	foreach name [$node attributes] {
	  lappend result $name
	}
      }
    }
    set result [join $result \n]
  }
  "mkdir" - "md" { # mkdir - Create an empty node
    set targetPath [PopArg]
    puts "Not implemented yet"
    exit 1
  }
  "mv" - "move" {	# mv - Move node or attribute
    set targetPath [PopArg]
    puts "Not implemented yet"
    exit 1
  }
  "rm" - "del" - "rmdir" - "rd" {  # Remove the node or attribute
    foreach node $nodes {
      if {"$attrib" == ""} { # We want to remove nodes
	set parent [$node parentNode]
	# Also remove the previous text node if it contains only spaces.
	set prevNode [$node previousSibling]
	if {"$prevNode" != ""} {
	  set prevType [$prevNode nodeType]
	  if {"$prevType" == "TEXT_NODE"} {
	    set prevValue [$prevNode nodeValue]
	    if [regexp {^\s*$} $prevValue -] {
	      $parent removeChild $prevNode
	    }
	  }
	}
	$parent removeChild $node
      } else {               # We want to remove attributes
	$node removeAttribute $attrib
      }
    }
    set result [$doc asXML] ; # Serialize the remaining data
  }
  "set" {   # Set the value of a node or attribute. 
    # WARNING: THIS COMMAND IS BROKEN AND NEEDS TO BE FIXED.
    if {"$attrib" == ""} { # We want to set a node
      set subdoc [dom parse $value]
      set subdocRoot [$subdoc documentElement]

      set subNodes [$subdocRoot selectNode "/*"]
      foreach node $nodes {
	if {   ("[string index $path end]" != "/")
	    && ([llength $subNodes] == 1)
	    && ("[$node nodeName]" == "[$subNodes nodeName]")
	   } {
	  # Then replace the target node
	  set newNode [::dom::document importNode $doc $subNodes -deep 1]
	  ::dom::node replaceChild [::dom::node parent $node] $newNode $node
	  continue
	}
	# Else append the data to the current children of the nodes.
	# First get information about the alignment of our future siblings..
	set firstChildIsBlank 0
	set lastChildIsBlank 0
	if [::dom::node hasChildNodes $node] {
	  set firstChild [$node firstChild]
	  set lastChild [$node lastChild]
	  set firstType [$firstChild nodeType]
	  set lastType [$lastChild nodeType]
	  if {("$firstType" == "textNode")} {
	    set firstValue [$firstChild nodeValue]
	    set firstChildIsBlank [regexp {^\s+$} $firstValue discard]
	  }
	  if {("$lastType" == "textNode")} {
	    set lastValue [$lastChild nodeValue]
	    set lastChildIsBlank [regexp {^\s+$} $lastValue discard]
	  }
	}
	# Then add the new nodes.
	foreach subNode $subNodes {
	  set newNode [::dom::document importNode $doc $subNode -deep 1]
	  # Try preserving the input document alignment if possible
	  if {$firstChildIsBlank && $lastChildIsBlank} {
	    # Then insert it with same alignment before the last one.
	    set newBlank [::dom::node cloneNode $firstChild -deep 1]
	    ::dom::node insertBefore $node $newBlank $lastChild
	    ::dom::node insertBefore $node $newNode $lastChild
	  } else {
	    # Default: Simply append it to the list
	    ::dom::node appendChild $node $newNode
	  }
	}
      }
    } else { # We want to set an attribute
      foreach node $nodes {
	# Note: setAttribute creates the attribute if needed
	::dom::element setAttribute $node $attrib $value
      }
    }
    set result [$doc asXML] ; # Serialize the remaining data
  }
  "touch" {          # touch - Create an empty attribute
    puts "Not implemented yet"
    exit 1
  }
  "tree" { # Dump the node tree structure
    if {"$path" == "/"} {
      set nodes [list $root]
    }
    foreach node $nodes {
      DumpNode $node [list #[info level] result] 0
    }
    set result [join $result \n]
  }
  "xml" - . {  # Display the nodes selected
    if {"$path" == "/"} {
      set nodes [list $root]
    }
    foreach node $nodes {
      # Prefix the last line from the previous text node if it contains only spaces.
      set prefix "" ; # This is useful for getting a sensible output indentation.
      set prevNode [$node previousSibling]
      if {"$prevNode" != ""} {
	set prevType [$prevNode nodeType]
	if {"$prevType" == "TEXT_NODE"} {
	  set prevValue [$prevNode nodeValue]
	  if [regexp {^\s*$} $prevValue discard] {
	    set prefix [lindex [split $prevValue \n] end]
	  }
	}
      }
      lappend result $prefix[$node asXML]
    }
    set result [join $result \n]
  }
  default {
    puts stderr "Unsupported command: $action"
    puts $usage
    exit 1
  }
}
# Output it
if {"$result" != ""} {
  puts $hOutput $result
}

# Cleanup
close $hInput
close $hOutput

