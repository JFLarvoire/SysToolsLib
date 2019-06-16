###############################################################################
#									      #
#   File name:	    driver.mak						      #
#									      #
#   Description:    Specific rules for building driver.exe.		      #
#									      #
#   Notes:	    							      #
#									      #
#   History:								      #
#    2015-12-09 JFL Created this file.					      #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#									      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=driver.c $(O)\driver.rc
OBJECTS=$(O)\driver.obj $(O)\driver.res
EXENAME=driver.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF

