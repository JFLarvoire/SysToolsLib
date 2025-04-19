#!/bin/sh
###############################################################################
#                                                                             #
#   Filename        mcd.sh                                                    #
#                                                                             #
#   Description     Define a routine that creates a directory, and enters it. #
#                                                                             #
#   Properties      jEdit local buffer properties: :encoding=utf-8:tabSize=4: #
#                                                                             #
#   Note            To have the routine available in every shell, put this    #
#                   script in /etc/profile.d/.                                #
#                                                                             #
#   History                                                                   #
#    2021-01-16 JFL Created this script.                                      #
#                                                                             #
#         © Copyright 2021 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

mcd() {
  mkdir -p "$1" && cd "$1"
}
