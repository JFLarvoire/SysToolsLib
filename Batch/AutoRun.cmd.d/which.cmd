@echo off
:##############################################################################
:#                                                                            #
:#  Filename        which.cmd                                                 #
:#                                                                            #
:#  Description     Automatically use (which.exe -i) to search doskey macros  #
:#                                                                            #
:#  Notes 	    By default, which.exe only searches for EXEs in the PATH. #
:#                  With option -i, it also searches for cmd.exe internal     #
:#                  commands, and doskey macros, passed via standard input.   #
:#                  As this sequence of commands is cumbersome to type, this  #
:#                  which.cmd doskey macro automates the use of (which -i).   #
:#                                                                            #
:#                  Run (which.exe -?) for help on the (which.exe -i) option. #
:#                                                                            #
:#  Authors         JFL Jean-François Larvoire, jf.larvoire@free.fr           #
:#                                                                            #
:#  History                                                                   #
:#   2019-02-18 JFL Created this script.                                      #
:#		                                                              #
:##############################################################################

doskey /macros which=^(help ^& doskey /macros^) ^| which.exe -i $*
