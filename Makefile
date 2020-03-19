###############################################################################
#                                                                             #
#  File name        Makefile                                                  #
#                                                                             #
#  Description      A GNU make (gmake) makefile to build all SysToolsLib      #
#                                                                             #
#  Notes            MUST BE EXECUTED BY GMAKE (GNU Make), NOT UNIX MAKE.      #
#                   Else the conditional directives won't work.               #
#                                                                             #
#  History                                                                    #
#    2020-03-19 JFL jf.larvoire@hpe.com created this file.                    #
#                                                                             #
#         © Copyright 2020 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Pass everything on to the main make file in the C directory
$(MAKECMDGOALS) an_unlikely_target_name_to_make_sure_theres_at_least_one_goal_here:
	@$(MAKE) -s -C C $(MFLAGS) $(MAKECMDGOALS)
