###############################################################################
#                                                                             #
#   File name       Get-Console.ps1                                           #
#                                                                             #
#   Description     Capture the console screen buffer as HTML, RTF, or Text   #
#                                                                             #
#   Notes           Known issues:                                             #
#                   - Double-width characters (Ex chinese) are recorded twice.#
#                   - Clipboard output requires PowerShell v3.                #
#                     Workaround for PowerShell v2:                           #
#			Get-Console.ps1 -Path - | & 2clip.exe -h              #
#                                                                             #
#   History                                                                   #
#    2009-01-11 VA  Published https://blogs.msdn.microsoft.com/powershell/2009/01/11/colorized-capture-of-console-screen-in-html-and-rtf/
#    2013-09-23 TD  Published https://blogs.msdn.microsoft.com/timid/2013/09/23/capturing-the-console-buffer/
#    2017-12-04 JFL Changed back to using VA's initial code for HTLM.         #
#                   Added function Append-NewLine(), and use it instead of    #
#		    Append-HtmlBreak to improve readability of the HTML output.
#   		    Also normalize the gray color.			      #
#   		    Added arguments -Lines, -Text, and -Window.		      #
#   		    Erase the Get-ConsoleAsHtml command line in the output.   #
#                   Moved the file output out of Get-ConsoleBuffer to main.   #
#    2018-01-08 JFL Fixed missing spaces in RTF output.                       #
#                   End lines with &nbsp; to fix pasting into Word or Outlook.#
#                   Added option -Trim to _not_ use the &nbsp; workaround.    #
#                   Use 2clip.exe for outputing to the clipboard.             #
#    2018-01-09 JFL Rewrote the clipboard output to use .NET APIs, removing   #
#                   the dependency on 2clip.exe.                              #
#                   Fixed Unicode characters in HTML and RTF output.          #
#    2018-01-10 JFL Use the same font and font size for HTML and RTF.         #
#                                                                             #
###############################################################################

<#
  .SYNOPSIS
  Capture the console screen buffer as HTML, RTF, or Text

  .DESCRIPTION
  This script captures the console screen buffer up to the current cursor position
  and outputs it in HTML or RTF or Text format to the clipboard or a file.
  Default: HTML output to the clipboard, optimized for pasting into Outlook.
  Also it removes its own invocation from the bottom line.

  .PARAMETER Lines
  Capture this number of lines at the bottom of the screen buffer.
  If negative, captures the top -$Lines lines of the screen buffer.
  Default: Capture just the visible window.
  Alias: -N

  .PARAMETER Path
  Output the captured data into this file.
  "-": Output to stdout
  Default: Output to the clipboard.

  .PARAMETER Window
  Capture the visible window. Default.

  .PARAMETER All
  Capture the whole screen buffer.

  .PARAMETER HTML
  Capture the requested area as HTML. Default.

  .PARAMETER RTF
  Capture the requested area as Rich Text Format.

  .PARAMETER Text
  Capture the requested area as plain text.

  .PARAMETER Trim
  Do not append a hard space to captured lines.
  By default, a hard space is appended to every HTML or RTF line, ensuring that
  trailing spaces are coloured correctly when pasted in Word or Outlook.
  With the -Trim switch, these hard spaces aren't appended, and the background
  colouring stops at the last printable character in Word or Outlook.
  In Text mode, -Trim removes trailing spaces from all lines.

  .PARAMETER Version
  Display this script version and exit.

  .EXAMPLE
  ./Get-Console
  Capture the visible window as HTML, and copy it to the clipboard.

  .EXAMPLE
  ./Get-Console 200 -Path screen.htm
  Capture the last 200 lines as HTML, and save that into the file screen.htm.
#>

Param (
  [Int][alias("N")]$Lines = 0,	# Number of lines to capture. 0 = Window or All.
  [string]$Path = $null,        # Optional output path. "-" = stdout. Default = clipboard
  [Switch]$Window,		# Capture the visible window (Default)
  [Switch]$All,			# Capture the whole screen buffer
  [Switch]$HTML,		# Capture the requested area as HTML (Default)
  [Switch]$RTF,			# Capture the requested area as RTF
  [Switch]$Text,		# Capture the requested area as text
  [Switch]$Trim,		# Do not append a hard space to captured lines
  [Switch]$Version		# Display the script version and exit
)

# If the -Version switch is specified, display the script version and exit.
$scriptVersion = "2018-01-10"
if ($Version) {
  echo $scriptVersion
  return
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-ConsoleBuffer                                         #
#                                                                             #
#   Description     Capture the console screen buffer as HTML, RTF, or Text   #
#                                                                             #
#   Arguments       See the Param() block                                     #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

function Get-ConsoleBuffer {
  param (
    [switch]$All,
    [switch]$HTML,
    [Int][alias("N")]$Lines = 0,	# Number of lines to capture. 0 = All.
    [switch]$RTF,
    [switch]$Text,
    [switch]$Trim,
    [switch]$Window
  )

  # Shared settings
  $fontName = 'Lucida Console'
  $fontSize = 10

  $crlf = "`r`n"

  # Check mutually exclusive options
  if (([int][bool]$HTML + [int][bool]$RTF + [int][bool]$Text) -gt 1) {
    throw "Incompatible options: -HTML, -RTF, -Text"
  }
  if (!$HTML -and !$RTF -and !$Text) { $HTML = $true }

  if (([int][bool]$All + [int]($Lines -ne 0) + [int][bool]$Window) -gt 1) {
    throw "Incompatible options: -All, -Lines, -Window"
  }
  if (!$All -and !$Lines -and !$Window) { $Window = $true }

  # HTML functions

  # The Windows PowerShell console host redefines DarkYellow and DarkMagenta colors and uses them as defaults.
  # The redefined colors do not correspond to the color names used in HTML, so they need to be mapped to digital color codes.
  function Normalize-HtmlColor($color) {
    switch ($color) {
      "DarkYellow"  { "#eeedf0" }
      "DarkMagenta" { "#012456" }
      "gray"        { "#C0C0C0" }
      default	  { $color }
    }
  }

  # Create an HTML span from text using the named console colors.
  function Make-HtmlSpan($text, $forecolor = "DarkYellow", $backcolor = "DarkMagenta") {
    $forecolor = Normalize-HtmlColor $forecolor
    $backcolor = Normalize-HtmlColor $backcolor
    # You can also add font-weight:bold tag here if you want a bold font in output.
    return "<span style='color:$forecolor;background:$backcolor'>$text</span>"
  }

  # Generate an HTML span and append it to HTML string builder
  function Append-HtmlSpan($Trim=$false) {
    $nbsp = if ($Trim) { "" } else { "&nbsp;" } # A hard space, forcing Word to color the end of line
    $spanText = $stringbuilder.ToString()
    $spanHtml = Make-HtmlSpan $spanText $currentForegroundColor $currentBackgroundColor
    $null = $htmlBuilder.Append($spanHtml + $nbsp)
  }

  # Append a new line to HTML builder
  function Append-NewLine {
    $null = $htmlBuilder.Append($crlf)
  }

  # End HTML functions

  # RTF functions

  function Get-RtfColorIndex ([string]$color) {
    switch ($color) {
      'DarkBlue'    {  2 }
      'DarkGreen'   {  3 }
      'DarkCyan'    {  4 }
      'DarkRed'     {  5 }
      'DarkMagenta' {  6 }
      'DarkYellow'  {  7 }
      'Gray'        {  8 }
      'DarkGray'    {  9 }
      'Blue'        { 10 }
      'Green'       { 11 }
      'Cyan'        { 12 }
      'Red'         { 13 }
      'Magenta'     { 14 }
      'Yellow'      { 15 }
      'White'       { 16 }
      'Black'       { 17 }
      default       {  0 }
    }
  }
  function New-RtfBuilder {
    $fontName = Get-Variable -Name fontName -Scope 1 -ValueOnly
    $fontSize = (Get-Variable -Name fontSize -Scope 1 -ValueOnly) * 2 # RTF uses a half-point unit

    # Initialize the RTF string builder.
    $script:rtfBuilder = new-object system.text.stringbuilder

    # Set the desired font
    & {
      # Append RTF header
      $script:rtfBuilder.Append("{\rtf1\fbidis\ansi\ansicpg1252\deff0\deflang1033{\fonttbl{\f0\fnil\fcharset0 $fontName;}}")
      $script:rtfBuilder.Append("`r`n")

      # Append RTF color table which will contain all Powershell console colors.
      # script version
      $script:rtfBuilder.Append('{\colortbl;red0\green0\blue128;\red0\green128\blue0;\red0\green128\blue128;\red128\green0\blue0;\red1\green36\blue86;\red238\green237\blue240;\red192\green192\blue192;\red128\green128\blue128;\red0\green0\blue255;\red0\green255\blue0;\red0\green255\blue255;\red255\green0\blue0;\red255\green0\blue255;\red255\green255\blue0;\red255\green255\blue255;\red0\green0\blue0;}')

      # Append RTF document settings.
      $script:rtfBuilder.Append("\viewkind4\uc1\pard\ltrpar\f0\fs$fontSize ")
    } | Out-Null
  }

  # Append line break to RTF builder
  function Add-RtfBreak($Trim=$false) {
    $nbsp = if ($Trim) { "" } else { "\~" } # \~ is a hard space, forcing Word to color the end of line
    $script:rtfBuilder.Append("{\shading0\cbpat$(Get-RtfColorIndex $currentBackgroundColor)$nbsp}\par`r`n") | Out-Null
  }

  # Append text to RTF builder
  function Add-RtfBlock($Text, $ForegroundColor = "DarkYellow", $BackgroundColor = "DarkMagenta") {
    $ForegroundColor= Get-RtfColorIndex $ForegroundColor
    $BackgroundColor= Get-RtfColorIndex $BackgroundColor

    $script:rtfBuilder.Append("{\cf$ForegroundColor") | Out-Null
    $script:rtfBuilder.Append("\chshdng0\chcbpat$BackgroundColor") | Out-Null
    # $script:rtfBuilder.Append(" $Text}") | Out-Null # The head space separates markup from text
    # Convert Unicode characters in Text to \uN RTF, else they're lost in the output
    $AsciiText = ""
    foreach ($c in $Text.GetEnumerator()) {
      $i = [int]$c
      if ($i -gt 127) {
	if ($i -gt 255) {
	  $c = "?"
	}
      	$c = "\u$i$c"
      }
      $AsciiText += $c
    }
    $script:rtfBuilder.Append(" $AsciiText}") | Out-Null # The head space separates markup from text
  }

  # End RTF functions

  # Core code of Get-ConsoleBuffer()

  # Check the host name and exit if the host is not the Windows PowerShell console host.
  if ($host.Name -ne 'ConsoleHost') {
    Write-Warning "$((Get-Variable -ValueOnly -Name MyInvocation).MyCommand)runs only in the console host. You cannot run this script in $($Host.Name)."
    return
  }

  # Initialize document name and object
  if ($HTML) {
    $htmlBuilder = new-object system.text.stringbuilder
    $null = $htmlBuilder.Append("<pre style='margin: 0; padding: 0; font-family: $fontName, Courier New, Courier; font-size: $($fontSize)pt; line-height: $($fontSize)pt'>" + $crlf)
    $RTF = $false
  } elseif ($RTF) {
    New-RtfBuilder
  } else { # if ($Text)
    $fileBuilder = New-Object system.Text.StringBuilder
  }

  # Grab the console screen buffer contents using the Host console API.
  $bufferWidth = $Host.UI.RawUI.BufferSize.Width
  $bufferHeight = $Host.UI.RawUI.CursorPosition.Y

  # Lines at which capture starts and ends
  $endY = $bufferHeight
  if ($Window) {
    $startY = $bufferHeight - $Host.UI.RawUI.WindowSize.Height
  } elseif ($All) { # Capture the window height
    $startY = 0
  } elseif ($Lines -gt 0) {
    $startY = $bufferHeight - $Lines
  } else { # $Lines < 0
    $startY = 0
    $endY = -$Lines
  }
  if ($startY -lt 0) { # Invalid input
    $startY = 0		# Start capture from the beginning
  }

  # Copy just the lines we want
  $rec =  New-Object System.Management.Automation.Host.Rectangle 0, $startY, ($bufferWidth - 1), $bufferHeight
  $buffer = $Host.UI.RawUI.GetBufferContents($rec)

  # Prepare to skip the Get-Console command on the last line
  $LastLine = ""
  $iLastLine = $bufferHeight-$startY-1
  for ($j = 0; $j -lt $bufferWidth; $j++) {
    $c = $buffer[$iLastLine, $j].Character
    $LastLine += "$c"
  }
  $jStopLast = $LastLine.IndexOf($script:MyInvocation.Line) # -1 if not found

  # Iterate through the lines in the console buffer.
  $utf8 = [System.Text.Encoding]::UTF8
  $ascii = [System.Text.Encoding]::ASCII
  for ($i = $startY; $i -lt $endY; $i++) {
    $iBuf = $i - $StartY

    $stringBuilder = New-Object System.Text.StringBuilder

    if ($HTML -or $RTF) {
      # Track the colors to identify spans of text with the same formatting.
      $currentForegroundColor = $buffer[$iBuf, 0].ForegroundColor
      $currentBackgroundColor = $buffer[$iBuf, 0].BackgroundColor
    }

    for ($j = 0; $j -lt $bufferWidth; $j++) {
      $cell = $buffer[$iBuf, $j]
      if (($iBuf -eq $iLastLine) -and ($jStopLast -gt 0) -and ($j -ge $jStopLast)) {
	$cell.Character = ' ' # Erase the Get-Console command line
      }

      # Check the character width (In an attempt to fix the double capture of full-size east-asian characters)
      # Commented out as this unfortunately always returns 1
      # $nCols = $Host.UI.RawUI.LengthInBufferCells($cell.Character)
      # if ($cell.Character -ne " ") {
      #   Write-Host "$($cell.Character) uses $nCols columns"
      # }
      # if ($nCols -gt 1) {
      #   Write-Host "$($cell.Character) uses $nCols columns"
      #   $j += $nCols - 1
      # }

      # If the colors change, generate an HTML span and append it to the HTML string builder.
      if (($HTML -or $RTF) -and (($cell.ForegroundColor -ne $currentForegroundColor) -or ($cell.BackgroundColor -ne $currentBackgroundColor))) {
	if ($HTML)    {
	  Append-HtmlSpan $true # $true = Do trim the optional hard space in the end
	  # Reset the span builder and colors.
	  $stringBuilder = new-object system.text.stringbuilder
	  $currentForegroundColor = $cell.Foregroundcolor
	  $currentBackgroundColor = $cell.Backgroundcolor
	} elseif ($RTF) {
	  Add-RtfBlock -Text $stringBuilder.ToString() -ForegroundColor $currentForegroundColor -BackgroundColor $currentBackgroundColor
	}

	# Reset the span builder and colors.
	$stringBuilder = New-Object System.Text.StringBuilder
	$currentForegroundColor = $cell.ForegroundColor
	$currentBackgroundColor = $cell.BackgroundColor
      }

      # Substitute characters which have special meaning in HTML.
      $c = $cell.Character
      if ($HTML) {
	switch ($c) {
	  '>' { $c = '&gt;' }
	  '<' { $c = '&lt;' }
	  '&' { $c = '&amp;' }
	}
	# $c = $ascii.GetString($utf8.Getbytes($c))
      } elseif ($RTF) {
	switch ($c)  {
	  "`t"    { $c = '\tab' }
	  '\'     { $c = '\\' }
	  '{'     { $c = '\{' }
	  '}'     { $c = '\}' }
	}
      }
      $stringBuilder.Append($c) | Out-Null
    }

    if ($HTML) {
      Append-HtmlSpan $Trim
      Append-NewLine  # No need to output an HTML break, as we're inside a PREformatted block.
    }
    elseif ($RTF) {
      Add-RtfBlock -Text $stringBuilder.ToString() -ForegroundColor $currentForegroundColor -BackgroundColor $currentBackgroundColor
      Add-RtfBreak -Trim:$Trim
    }
    else { # if ($Text)
      $String = $stringBuilder.ToString()
      if ($Trim) { $String = $String.TrimEnd() }
      $fileBuilder.Append($String + $crlf) | Out-Null
    }
  }

  # End the document
  if ($HTML) {
    # Append HTML ending tag.
    $null = $htmlBuilder.Append("</pre>")
    "<html>$crlf<head>$crlf</head>$crlf<body>$crlf" + $htmlBuilder.ToString() + "$crlf</body>$crlf</html>"
  } elseif ($RTF)  {
    $script:rtfBuilder.ToString() + '}'
  } else { # if ($Text)
    $fileBuilder.ToString() + $crlf
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        ConvertTo-HtmlClipboardFormat                             #
#                                                                             #
#   Description     Convert HTML string to the format used in the clipboard   #
#                                                                             #
#   Arguments                                                                 #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2018-01-09 JFL Created this routine                                      #
#                                                                             #
#-----------------------------------------------------------------------------#

function ConvertTo-HtmlClipboardFormat($RawHtml) {
  # Convert the captured string to a UTF8 string, as expected by the clipboard
    # $utf8 = [System.Text.Encoding]::UTF8
    # $bytes = $utf8.Getbytes($RawHtml)
    # $HtmlString = "" # UTF8 string
    # foreach ($b in $bytes.GetEnumerator()) { $HtmlString += [char]$b }
    # Surprisingly, this works for many Unicode characters, but not all.
  # Instead, convert non-ASCII character in the captured string to HTML entities
  $HtmlString = "" # ASCII string with HTML entities
  foreach ($c in $RawHtml.GetEnumerator()) {
    $i = [int]$c
    if ($i -gt 127) {
      $c = "&#$i;"
    }
    $HtmlString += $c
  }
  # Now build the header that must be inserted ahead of the HTML fragment to put into the clipboard
  # The header length varies, depending on the number of digits in each of the following numbers
  $nHead = 10 # The header length. We know it will be between 10 and 100. So use 10 as a seed.
  $nCaptured = $HtmlString.Length
  $iFragment = $HtmlString.IndexOf("<body")
  $iFragment = $HtmlString.IndexOf(">", $iFragment) + 1
  $iEndFragment = $HtmlString.IndexOf("</body")
  do { # Inserting the values in the header will likely grow the header size. So repeat this until the size stabilizes.
    $nHead0 = $nHead
    $header = "Version:1.0`r`nStartHTML:$nHead`r`nEndHTML:$($nHead+$nCaptured)`r`nStartFragment:$($nHead+$iFragment)`r`nEndFragment:$($nHead+$iEndFragment)`r`n"
    $nHead = $header.Length
  } while ($nHead -gt $nHead0)
  return $header + $HtmlString
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Main                                                      #
#                                                                             #
#   Description     Execute the specified actions                             #
#                                                                             #
#   Arguments       See the Param() block at the top of this script           #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

# if we execute this file instead of dot-source it 
if ($MyInvocation.invocationName -ne '.') {
  # All error checking for mutually exclusive options is done within Get-ConsoleBuffer()
  if (!$HTML -and !$RTF -and !$Text) { $HTML = $true } # But we need this one further down

  $captured = Get-ConsoleBuffer -All:$All -Lines $Lines -Html:$HTML -RTF:$RTF -Text:$Text -Trim:$Trim -Window:$Window
  # TODO: Known bug: Double-width characters (ex: chinese) are captured twice

  if ($path) {
    if ($path -eq "-") {
      $captured
    } else {
      $captured | Out-File -FilePath $Path -Encoding utf8
    }
  } else { # No output file. Output to the clipboard
    # Set-Clipboard -AsHtml:$HTML $captured
    # Works fine with ASCII, but not with non-ASCII text.
    # Also requires PowerShell 5.0.
    # And does not support sending RTF to the clipboard.

    # Instead, use .NET APIs, that work in any PowerShell version
    # https://msdn.microsoft.com/en-us/library/system.windows.forms.clipboard.aspx
    # https://brianreiter.org/2010/09/03/copy-and-paste-with-clipboard-from-powershell/
    # http://powershell-tips.blogspot.de/2011/05/handling-clipboard-with-powershell.html
    Add-Type -AssemblyName system.windows.forms
    if ($HTML) {
      $format = [System.Windows.Forms.TextDataFormat]::Html
      $string = ConvertTo-HtmlClipboardFormat $captured
    } elseif ($RTF) {
      $format = [System.Windows.Forms.TextDataFormat]::Rtf
      $string = $captured
    } else { # if ($Text)
      $format = [System.Windows.Forms.TextDataFormat]::UnicodeText
      $string = $captured
    }
    [System.Windows.Forms.Clipboard]::SetText($string, $format)
  }
}
