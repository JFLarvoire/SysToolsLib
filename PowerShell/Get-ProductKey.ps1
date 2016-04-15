##########################################################
#
# Get-ProductKey.ps1
#     Description : récupère la clé produit de Windows
#
#    http://www.powershell-scripting.com/
#    La communauté PowerShell francophone
#
# Usage : PS > ./Get-ProductKey.ps1
##########################################################

# Création de la table de conversion base 24 
$map="BCDFGHJKMPQRTVWXY2346789" 

# Lecture de la clé de registre 
$value = (get-itemproperty "HKLM:\\SOFTWARE\Microsoft\Windows NT\CurrentVersion").digitalproductid[0x34..0x42] 

# Conversion des valeurs en Hexa pour afficher le Raw Key $hexa = ""
$value | foreach {
  $hexa = $_.ToString("X2") + $hexa
}
Write-Output "Raw Key Big Endian: $hexa" 

# Calcul du Product Key 
$ProductKey = ""
for ($i = 24; $i -ge 0; $i--) {
  $r = 0
  for ($j = 14; $j -ge 0; $j--) {
    $r = ($r * 256) -bxor $value[$j]
    $value[$j] = [math]::Floor([double]($r/24))
    $r = $r % 24
  }
  $ProductKey = $map[$r] + $ProductKey 
  if (($i % 5) -eq 0 -and $i -ne 0) {
    $ProductKey = "-" + $ProductKey
  }
}
Write-Output "Product Key: $ProductKey"
