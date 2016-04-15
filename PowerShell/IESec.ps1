###############################################################################
#                                                                             #
#   File name       IESec.ps1                                                 #
#                                                                             #
#   Description     Test Internet Explorer Enhanced Security configuration    #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-06-09 JFL Created this script.                                      #
#                                                                             #
###############################################################################

$keyAdmin = "HKLM:\SOFTWARE\Microsoft\Active Setup\Installed Components\{A509B1A7-37EF-4b3f-8CFC-4F3A74704073}"
$iAdmin = (Get-ItemProperty -ErrorAction SilentlyContinue $keyAdmin).IsInstalled
if ($iAdmin) {
  echo "Internet Explorer Enhanced Security Configuration is ON for Administrators."
} else {
  echo "Internet Explorer Enhanced Security Configuration is OFF for Administrators."
}

$keyUsers = "HKLM:\SOFTWARE\Microsoft\Active Setup\Installed Components\{A509B1A8-37EF-4b3f-8CFC-4F3A74704073}"
$iUsers = (Get-ItemProperty -ErrorAction SilentlyContinue $keyUsers).IsInstalled
if ($iUsers) {
  echo "Internet Explorer Enhanced Security Configuration is ON for Users."
} else {
  echo "Internet Explorer Enhanced Security Configuration is OFF for Users."
}
