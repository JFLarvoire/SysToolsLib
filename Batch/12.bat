:##############################################################################
:#                                                                            #
:#  Filename        12.bat	                                              #
:#                                                                            #
:#  Description     Pipe clipboard data through a command and back            #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2006-10-21 JFL Created this script.		                      #
:#   2014-04-18 JFL Added the new -A options to both 1clip and 2clip, as data #
:#                  from GUI programs is likely to be encoded as ANSI, and    #
:#                  some of my filter programs still only support ANSI data.  #
:#                                                                            #
:##############################################################################
1clip -A | %* | 2clip -A
