@echo off
:##############################################################################
:#                                                                            #
:#  Filename        history.bat                                               #
:#                                                                            #
:#  Description     Emulate the Unix shells history command		      #
:#                                                                            #
:#  Notes 	    The Windows cmd.exe shell history is managed by the       #
:#		    'doskey /history' command.                                #
:#                                                                            #
:#		    But unfortunately, it's not sufficient to write a simple  #
:#		    batch file containing just 'doskey /history'. The problem #
:#                  with this approach is that if you try to pipe the output  #
:#                  of that batch to another command, for example findstr.exe,#
:#                  then this executes history.bat in a sub-shell... which    #
:#                  has an empty history.                                     #
:#                                                                            #
:#		    To avoid that, it's necessary to define a doskey macro.   #
:#                  Contraty to a batch file, a macro is execucted in the     #
:#                  context of the current shell, and so reports the correct  #
:#                  history.                                                  #
:#                                                                            #
:#		    Installation: Execute this script once to create the      #
:#		    history macro. This displays nothing.		      #
:#		    Subsequent uses of the history macro will display the     #
:#		    shell history as expected.                                #
:#                                                                            #
:#                  Recommended installation: Use it in combination with the  #
:#                  AutoRun.cmd script. Put it in the AutoRun.cmd.d folder:   #
:#                  copy history.bat "%ALLUSERSPROFILE%\AutoRun.cmd.d\"       #
:#                  Then history.bat will automatically be initialized when   #
:#                  new cmd.exe shells are opened.                            #
:#                  Run 'AutoRun.cmd -?' to get more information about that.  #
:#                                                                            #
:#  Authors         JFL Jean-François Larvoire, jf.larvoire@free.fr           #
:#                                                                            #
:#  History                                                                   #
:#   2019-02-03 JFL Created this script.                                      #
:#		                                                              #
:##############################################################################

doskey history=doskey /history $*
