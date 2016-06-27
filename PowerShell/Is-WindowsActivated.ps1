#-----------------------------------------------------------------------------#
#                                                                             #
#   File name       Is-WindowsActivated.ps1	                              #
#                                                                             #
#   Description     Check if Windows is activated		              #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2012-08-23 JFL Created this script.				      #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
#-----------------------------------------------------------------------------#

# Script to check the Windows activation status
Function ActivationStatus($code) {
  switch($code) {
    0 {"Unlicensed"}
    1 {"Licensed"}
    2 {"Out-Of-Box Grace Period"}
    3 {"Out-Of-Tolerance Grace Period"}
    4 {"Non-Genuine Grace Period"}
    5 {"Notification"}
    6 {"Extended Grace"}
    default {"Unknown value"}
  }
}

# Using the Where filter to just determine if the product has been licensed or not
$wpa = Get-WmiObject -class SoftwareLicensingProduct | Where {$_.LicenseStatus -eq "1"}

if ($wpa) {
  foreach($item in $wpa) {
    $status = ActivationStatus($item.LicenseStatus)
    "Windows Activation Status: {0}" -f $status
  }
} else {
  "Windows Activation Status: Unlicensed"
}

