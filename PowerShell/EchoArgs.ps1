# EchoArgs.ps1 - Echo all command line arguments.

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Quote                                                     #
#                                                                             #
#   Description     Put quotes around a string, if needed for reinterpretation#
#                                                                             #
#   Notes           Quoting constraints:                                      #
#                   - 'strings' contents are left unchanged.                  #
#                   - "strings" contents special characters are interpreted.  #
#                   - We try to minimize the number of ` escape sequences.    #
#                   - We prefer "strings" first (Looking more natural),       #
#                     then 'strings' (Possibly for a second level of quoting).#
#                                                                             #
#   History                                                                   #
#    2010-06-08 JFL Created this routine.                                     #
#    2010-06-17 JFL Added special handling for $null and arrays.              #
#    2010-06-24 JFL Added special handling for hash tables.                   #
#    2010-07-23 JFL Added special handling for booleans.                      #
#    2013-07-19 JFL Added special handling for blocks of code.                #
#    2013-10-07 JFL Added special handling for Parameter lists.               #
#    2013-11-18 JFL Added special handling for enumerations and types.        #
#                   Display the default fields for objects that have an       #
#                   explicit or default format definition.                    #
#                                                                             #
#-----------------------------------------------------------------------------#

$ImplicitTypes = "System.String", "System.Decimal", "System.Single", "System.Double",
                 "System.Char", "System.Int16", "System.Int32", "System.Int64",
                 "System.Byte", "System.UInt16", "System.UInt32", "System.UInt64"

Function Quote(
  $var, # Don't set $var type at this stage, to catch $null, etc.
  [Switch]$Force # $True=Quote all strings; $false=Only when necessary
) {
  if ($var -eq $null) { # Special case of the $null object
    return '$null'
  }
  if (($var -is [Boolean]) -or ($var -is [System.Management.Automation.SwitchParameter])) { # Special case of booleans
    if ($var) {
      return '$True'
    } else {
      return '$False'
    }
  }
  if ($var -is [Array]) { # This is an array. Return a list of quoted array members.
    return "@($(($var | foreach { Quote $_ -Force }) -join ", "))"
  }
  if ($var -is [Hashtable]) { # This is a hash table. Sort keys which are ordered randomly by hash.
    return "@{$(($var.Keys | sort | foreach { "$_ = $(Quote $var.$_ -Force )" }) -join "; ")}"
  }
  if ($var -is [Enum]) { # This is an enumeration. Force quoting string values to avoid issues with object members (which are often enums).
    return Quote "$var" -Force
  }
  if ($var -is [type]) { # This is a type. Return its name as a cast.
    return "[$($var.Name)]"
  }
  if ($var -is [ScriptBlock]) { # This is a block of code. Return it in curly brackets.
    return "{$var}"
  }
  $type = $var.psTypeNames[0] # Try using this type name, which is sometimes more descriptive than the official one in GetType().FullName
  if ($type -eq $null) { # $type seems to be always defined, but just in case, if it's not, fallback to the documented name.
    $type = $var.GetType().FullName
  }
  if (    $type -eq "System.Management.Automation.PSBoundParametersDictionary" `
      -or $type -like "System.Collections.Generic.Dictionary*") { # This is a dictionary. Keys are ordered already.
    return "@{$(($var.Keys | foreach { "$_ = $(Quote $var.$_ -Force)" }) -join "; ")}"
  }
  if (!($ImplicitTypes -contains $type)) { # If this is not a simple type in the list above
    $values = @()
    if ($var.PSStandardMembers.DefaultDisplayPropertySet.ReferencedPropertyNames) {
      # This object has explicit display properties defined. Use them.
      foreach ($name in $var.PSStandardMembers.DefaultDisplayPropertySet.ReferencedPropertyNames) {
    $value = Quote $var.$name
    $values += "$name = $value"
      }
    } else { # Check if the type has a default *.ps1xml format data definition
      $fd = Get-FormatData $type # If type strings in .psTypeNames[0] and .GetType().FullName differ, it's the first one that gives good results.
      if ($fd -and $fd.FormatViewDefinition[0].control.Entries.Items) {
    # We do have a list of default fields to display. (The ones used by Out-String by default!)
    foreach ($item in $fd.FormatViewDefinition[0].control.Entries.Items) {
      switch ($item.DisplayEntry.ValueType) {
        "Property" {
          $name = $item.DisplayEntry.Value
          $value = Quote $var.$name
        }
        "ScriptBlock" {
          $name = $item.Label
          $value = Quote (eval "`$_ = `$var ; $($item.DisplayEntry.Value)")
        }
        "default" {
          Write-Error "Unsupported ValueType: $($item.DisplayEntry.ValueType)"
        }
      }
      $values += "$name = $value"
    }
      }
    }
    switch ($values.length) {
      0 {} # No type list found. Fall through into the [string] cast default.
      1 {return $value} # Trivial object with just one field. No need to specify type and detailed field names since conversion will be trivial.
      default {return "[$type]@{$($values -join "; ")}"} # Complex object with multiple fields. Report type and every field with a [PSCustomObject]-like syntax.
    }
    # Else let the [string] cast do the conversion
    $Force = $True # Force quotes around it, else the type cast will likely fail.
    $TypeCast = "[$type]"
  } else {
    $TypeCast = ""
  }
  $string = [string]$var # Now whatever the type, convert it to a real string
  if ($string.length -eq 0) { # Special case of the empty string
    return '""'
  }
  if ($Force -or ($string -match "[ ``""'$]")) { # If there's any character that needs quoting
    if (($string -match '"') -and !($string -match "'")) { # If there are "s and no 's
      $string = "'$string'" # Surround with 's to preserve everything else.
    } else { # Either there are 's, or there are neither 's nor "s
      $s2 = ''
      for ($i=0; $i -lt $string.length; $i++) {
    $s2 += Switch ($string.Chars($i)) {
      '`' { '``'; break }
      '$' { '`$'; break }
      '"' { '""'; break }
      default { $_; break }
    }
      }
      $string = """$s2"""
    }
  }
  return "$TypeCast$string"
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Main                                                      #
#                                                                             #
#   Description     Execute the selected action                               #
#                                                                             #
#   Arguments                                                                 #
#                                                                             #
#   Notes                                                                   #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

$n = 0
foreach ($arg in $args) {
  $n += 1
  echo "Arg$n = $(Quote $Arg)"
}
