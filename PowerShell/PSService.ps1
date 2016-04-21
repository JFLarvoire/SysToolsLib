###############################################################################
#                                                                             #
#   File name       PSService.ps1                                             #
#                                                                             #
#   Description     A sample service in a standalone PowerShell script        #
#                                                                             #
#   Notes           The latest PSService.ps1 version will be available in     #
#                   https://github.com/JFLarvoire/SysToolsLib/ , in the       #
#                   PowerShell subdirectory.                                  #
#                   Please report any problem in the Issues tab in that       #
#                   GitHub repository.                                        #
#                                                                             #
#                   Dynamically generates a small PSService.exe wrapper       #
#                   application, that in turn invokes this PowerShell script. #
#                                                                             #
#                   Some arguments are inspired by Linux' service management  #
#                   arguments: -Start, -Stop, -Restart, -Status               #
#                   Others are more in the Windows' style: -Setup, -Remove    #
#                                                                             #
#                   The actual start and stop operations are done when        #
#                   running as SYSTEM, under the control of the SCM (Service  #
#                   Control Manager).                                         #
#                                                                             #
#                   To create your own service, make a copy of this file and  #
#                   rename it. The file base name becomes the service name.   #
#                   Then implement your own service code in the if ($Service) #
#                   {block} at the very end of this file. See the TO DO       #
#                   comment there.                                            #
#                   There are global settings below the script param() block. #
#                   They can easily be changed, but the defaults should be    #
#                   suitable for most projects.                               #
#                                                                             #
#                   Service installation and usage: See the dynamic help      #
#                   section below, or run: help .\PSService.ps1 -Detailed     #
#                                                                             #
#   History                                                                   #
#    2015-07-10 JFL jf.larvoire@hpe.com created this script.                  #
#    2015-10-13 JFL Made this script completely generic, and added comments   #
#                   in the header above.                                      #
#    2016-01-02 JFL Moved the Event Log name into new variable $logName.      #
#                   Improved comments.                                        #
#    2016-01-05 JFL Fixed the StartPending state reporting.                   #
#    2016-03-17 JFL Removed aliases. Added missing explicit argument names.   #
#    2016-04-16 JFL Moved the official repository on GitHub.                  #
#    2016-04-21 JFL Minor bug fix: New-EventLog did not use variable $logName.#
#                                                                             #
###############################################################################
#Requires -version 2

<#
  .SYNOPSIS
    A sample Windows service, in a standalone PowerShell script.

  .DESCRIPTION
    This script demonstrates how to write a Windows service in pure PowerShell.
    It dynamically generates a small PSService.exe wrapper, that in turn
    invokes this PowerShell script again for its start and stop events.

  .PARAMETER Start
    Start the service.

  .PARAMETER Stop
    Stop the service.

  .PARAMETER Restart
    Stop then restart the service.

  .PARAMETER Status
    Get the current service status: Not installed / Stopped / Running

  .PARAMETER Setup
    Install the service.

  .PARAMETER Remove
    Uninstall the service.

  .PARAMETER Service
    Run the service in the background. Used internally by the script.
    Do not use, except for test purposes.

  .PARAMETER Version
    Display this script version and exit.

  .EXAMPLE
    # Setup the service and run it for the first time
    C:\PS>.\PSService.ps1 -Status
    Not installed
    C:\PS>.\PSService.ps1 -Setup
    C:\PS># At this stage, a copy of PSService.ps1 is present in the path
    C:\PS>PSService -Status
    Stopped
    C:\PS>PSService -Start
    C:\PS>PSService -Status
    Running
    C:\PS># Load the log file in Notepad.exe for review
    C:\PS>notepad ${ENV:windir}\Logs\PSService.log

  .EXAMPLE
    # Stop the service and uninstall it.
    C:\PS>PSService -Stop
    C:\PS>PSService -Status
    Stopped
    C:\PS>PSService -Remove
    C:\PS># At this stage, no copy of PSService.ps1 is present in the path anymore
    C:\PS>.\PSService.ps1 -Status
    Not installed
#>

[CmdletBinding(DefaultParameterSetName='Status')]
Param(
  [Parameter(ParameterSetName='Start', Mandatory=$true)]
  [Switch]$Start,       # Start the service

  [Parameter(ParameterSetName='Stop', Mandatory=$true)]
  [Switch]$Stop,        # Stop the service

  [Parameter(ParameterSetName='Restart', Mandatory=$true)]
  [Switch]$Restart,     # Restart the service

  [Parameter(ParameterSetName='Status', Mandatory=$false)]
  [Switch]$Status = $($PSCmdlet.ParameterSetName -eq 'Status'), # Get the current service status

  [Parameter(ParameterSetName='Setup', Mandatory=$true)]
  [Switch]$Setup,       # Install the service

  [Parameter(ParameterSetName='Remove', Mandatory=$true)]
  [Switch]$Remove,      # Uninstall the service

  [Parameter(ParameterSetName='Service', Mandatory=$true)]
  [Switch]$Service,     # Run the service

  [Parameter(ParameterSetName='Version', Mandatory=$true)]
  [Switch]$Version      # Get this script version
)

$scriptVersion = "2016-04-21"

# This script name, with various levels of details
$argv0 = Get-Item $MyInvocation.MyCommand.Definition
$script = $argv0.basename               # Ex: PSService
$scriptName = $argv0.name               # Ex: PSService.ps1
$scriptFullName = $argv0.fullname       # Ex: C:\Temp\PSService.ps1

# Global settings
$serviceName = $script                  # A one-word name used for net start commands
$serviceDisplayName = "A Sample PowerShell Service"
# $installDir = "${ENV:ProgramFiles}\$serviceName" # Where to install the service files
$installDir = "${ENV:windir}\System32"  # Where to install the service files
$scriptCopy = "$installDir\$scriptName"
$exeName = "$serviceName.exe"
$exeFullName = "$installDir\$exeName"
$logDir = "${ENV:windir}\Logs"          # Where to log the service messages
$logFile = "$logDir\$serviceName.log"
$logName = "Application"                # Event Log name (Unrelated to the logFile!)

# If the -Version switch is specified, display the script version and exit.
if ($Version) {
  Write-Output $scriptVersion
  return
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Now                                                       #
#                                                                             #
#   Description     Get a string with the current time.                       #
#                                                                             #
#   Notes           The output string is in the ISO 8601 format, except for   #
#                   a space instead of a T between the date and time, to      #
#                   improve the readability.                                  #
#                                                                             #
#   History                                                                   #
#    2015-06-11 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Now {
  Param (
    [Switch]$ms,        # Append milliseconds
    [Switch]$ns         # Append nanoseconds
  )
  $Date = Get-Date
  $now = ""
  $now += "{0:0000}-{1:00}-{2:00} " -f $Date.Year, $Date.Month, $Date.Day
  $now += "{0:00}:{1:00}:{2:00}" -f $Date.Hour, $Date.Minute, $Date.Second
  $nsSuffix = ""
  if ($ns) {
    if ("$($Date.TimeOfDay)" -match "\.\d\d\d\d\d\d") {
      $now += $matches[0]
      $ms = $false
    } else {
      $ms = $true
      $nsSuffix = "000"
    }
  } 
  if ($ms) {
    $now += ".{0:000}$nsSuffix" -f $Date.MilliSecond
  }
  return $now
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Log                                                       #
#                                                                             #
#   Description     Log a string into the PSService.log file                  #
#                                                                             #
#   Arguments       A string                                                  #
#                                                                             #
#   Notes           Prefixes the string with a timestamp and the user name.   #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Log ([String]$string) {
  if (!(Test-Path $logDir)) {
    New-Item -ItemType directory -Name $logDir
  }
  "$(Now) $userName $string" | Out-File -Encoding ASCII -Append "$logFile"
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        $source                                                   #
#                                                                             #
#   Description     C# source of the PSService.exe stub                       #
#                                                                             #
#   Arguments                                                                 #
#                                                                             #
#   Notes           The lines commented with "SET STATUS" and "EVENT LOG" are #
#                   optional. (Or blocks between "// SET STATUS [" and        #
#                   "// SET STATUS ]" comments.)                              #
#                   SET STATUS lines are useful only for services with a long #
#                   startup time.                                             #
#                   EVENT LOG lines are useful for debugging the service.     #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

$scriptCopyCname = $scriptCopy -replace "\\", "\\" # Double backslashes. (The first \\ is a regexp with \ escaped; The second is a plain string.)
$source = @"
  using System;
  using System.ServiceProcess;
  using System.Diagnostics;
  using System.Runtime.InteropServices;                                 // SET STATUS
  using System.ComponentModel;                                          // SET STATUS

  public enum ServiceType : int {                                       // SET STATUS [
    SERVICE_WIN32_OWN_PROCESS = 0x00000010,
    SERVICE_WIN32_SHARE_PROCESS = 0x00000020,
  };                                                                    // SET STATUS ]

  public enum ServiceState : int {                                      // SET STATUS [
    SERVICE_STOPPED = 0x00000001,
    SERVICE_START_PENDING = 0x00000002,
    SERVICE_STOP_PENDING = 0x00000003,
    SERVICE_RUNNING = 0x00000004,
    SERVICE_CONTINUE_PENDING = 0x00000005,
    SERVICE_PAUSE_PENDING = 0x00000006,
    SERVICE_PAUSED = 0x00000007,
  };                                                                    // SET STATUS ]

  [StructLayout(LayoutKind.Sequential)]                                 // SET STATUS [
  public struct ServiceStatus {
    public ServiceType dwServiceType;
    public ServiceState dwCurrentState;
    public int dwControlsAccepted;
    public int dwWin32ExitCode;
    public int dwServiceSpecificExitCode;
    public int dwCheckPoint;
    public int dwWaitHint;
  };                                                                    // SET STATUS ]

  public enum Win32Error : int { // WIN32 errors that we may need to use
    NO_ERROR = 0,
    ERROR_APP_INIT_FAILURE = 575,
    ERROR_FATAL_APP_EXIT = 713,
    ERROR_SERVICE_NOT_ACTIVE = 1062,
    ERROR_EXCEPTION_IN_SERVICE = 1064,
    ERROR_SERVICE_SPECIFIC_ERROR = 1066,
    ERROR_PROCESS_ABORTED = 1067,
  };

  public class $serviceName : ServiceBase {
    private System.Diagnostics.EventLog eventLog;                       // EVENT LOG
    private ServiceStatus serviceStatus;                                // SET STATUS

    public $serviceName() {
      ServiceName = "$serviceName";
      CanStop = true;
      CanPauseAndContinue = false;
      AutoLog = true;

      eventLog = new System.Diagnostics.EventLog();                     // EVENT LOG [
      if (!System.Diagnostics.EventLog.SourceExists(ServiceName)) {         
        System.Diagnostics.EventLog.CreateEventSource(ServiceName, "$logName");
      }
      eventLog.Source = ServiceName;
      eventLog.Log = "$logName";                                        // EVENT LOG ]
      EventLog.WriteEntry(ServiceName, "$exeName $serviceName()");      // EVENT LOG
    }

    [DllImport("advapi32.dll", SetLastError=true)]                      // SET STATUS
    private static extern bool SetServiceStatus(IntPtr handle, ref ServiceStatus serviceStatus);

    protected override void OnStart(string [] args) {
      EventLog.WriteEntry(ServiceName, "$exeName OnStart() // Entry. Starting script '$scriptCopyCname' -Start"); // EVENT LOG
      // Set the service state to Start Pending.                        // SET STATUS [
      // Only useful if the startup time is long. Not really necessary here for a 2s startup time.
      serviceStatus.dwServiceType = ServiceType.SERVICE_WIN32_OWN_PROCESS;
      serviceStatus.dwCurrentState = ServiceState.SERVICE_START_PENDING;
      serviceStatus.dwWin32ExitCode = 0;
      serviceStatus.dwWaitHint = 2000; // It takes about 2 seconds to start PowerShell
      SetServiceStatus(ServiceHandle, ref serviceStatus);               // SET STATUS ]
      // Start a child process with another copy of this script
      try {
        Process p = new Process();
        // Redirect the output stream of the child process.
        p.StartInfo.UseShellExecute = false;
        p.StartInfo.RedirectStandardOutput = true;
        p.StartInfo.FileName = "PowerShell.exe";
        p.StartInfo.Arguments = "-c & '$scriptCopyCname' -Start"; // Works if path has spaces, but not if it contains ' quotes.
        p.Start();
        // Read the output stream first and then wait. (To avoid deadlocks says Microsoft!)
        string output = p.StandardOutput.ReadToEnd();
        // Wait for the completion of the script startup code, that launches the -Service instance
        p.WaitForExit();
        if (p.ExitCode != 0) throw new Win32Exception((int)(Win32Error.ERROR_APP_INIT_FAILURE));
        // Success. Set the service state to Running.                   // SET STATUS
        serviceStatus.dwCurrentState = ServiceState.SERVICE_RUNNING;    // SET STATUS
      } catch (Exception e) {
        EventLog.WriteEntry(ServiceName, "$exeName // Failed to start $scriptCopyCname. " + e.Message, EventLogEntryType.Error); // EVENT LOG
        // Change the service state back to Stopped.                    // SET STATUS [
        serviceStatus.dwCurrentState = ServiceState.SERVICE_STOPPED;
        Win32Exception w32ex = e as Win32Exception; // Try getting the WIN32 error code
        if (w32ex == null) { // Not a Win32 exception, but maybe the inner one is...
          w32ex = e.InnerException as Win32Exception;
        }    
        if (w32ex != null) {    // Report the actual WIN32 error
          serviceStatus.dwWin32ExitCode = w32ex.NativeErrorCode;
        } else {                // Make up a reasonable reason
          serviceStatus.dwWin32ExitCode = (int)(Win32Error.ERROR_APP_INIT_FAILURE);
        }                                                               // SET STATUS ]
      } finally {
        serviceStatus.dwWaitHint = 0;                                   // SET STATUS
        SetServiceStatus(ServiceHandle, ref serviceStatus);             // SET STATUS
        EventLog.WriteEntry(ServiceName, "$exeName OnStart() // Exit"); // EVENT LOG
      }
    }

    protected override void OnStop() {
      EventLog.WriteEntry(ServiceName, "$exeName OnStop() // Entry");   // EVENT LOG
      // Start a child process with another copy of ourselves
      Process p = new Process();
      // Redirect the output stream of the child process.
      p.StartInfo.UseShellExecute = false;
      p.StartInfo.RedirectStandardOutput = true;
      p.StartInfo.FileName = "PowerShell.exe";
      p.StartInfo.Arguments = "-c & '$scriptCopyCname' -Stop"; // Works if path has spaces, but not if it contains ' quotes.
      p.Start();
      // Read the output stream first and then wait.
      string output = p.StandardOutput.ReadToEnd();
      // Wait for the PowerShell script to be fully stopped.
      p.WaitForExit();
      // Change the service state back to Stopped.                      // SET STATUS
      serviceStatus.dwCurrentState = ServiceState.SERVICE_STOPPED;      // SET STATUS
      SetServiceStatus(ServiceHandle, ref serviceStatus);               // SET STATUS
      EventLog.WriteEntry(ServiceName, "$exeName OnStop() // Exit");    // EVENT LOG
    }

    public static void Main() {
      System.ServiceProcess.ServiceBase.Run(new $serviceName());
    }
  }
"@

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Main                                                      #
#                                                                             #
#   Description     Execute the specified actions                             #
#                                                                             #
#   Arguments       See the Param() block at the top of this script           #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

# Check if we're running as a real user, or as the SYSTEM = As a service
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$userName = $identity.Name      # Ex: "NT AUTHORITY\SYSTEM" or "Domain\Administrator"
$authority,$name = $username -split "\\"
$isSystem = ($userName -eq "NT AUTHORITY\SYSTEM")
# Log "# `$userName = `"$userName`" ; `$isSystem = $isSystem"

Log $MyInvocation.Line # The exact command line that was used to start us

# The following commands write to the event log, but we need to make sure the PSService source is defined.
New-EventLog -LogName $logName -Source $serviceName -ea SilentlyContinue

if ($Start) {                   # Start the service
  if ($isSystem) { # If running as SYSTEM, ie. invoked as a service
    # Do whatever is necessary to start the service script instance
    Log "$scriptName -Start: Starting script '$scriptFullName' -Service"
    Write-EventLog -LogName $logName -Source $serviceName -EventId 1001 -EntryType Information -Message "$scriptName -Start: Starting script '$scriptFullName' -Service"
    Start-Process PowerShell.exe -ArgumentList ("-c & '$scriptFullName' -Service")
  } else {
    Write-Verbose "Starting service $serviceName"
    Write-EventLog -LogName $logName -Source $serviceName -EventId 1002 -EntryType Information -Message "$scriptName -Start: Starting service $serviceName"
    Start-Service $serviceName # Ask Service Control Manager to start it
  }
  return
}

if ($Stop) {                    # Stop the service
  if ($isSystem) { # If running as SYSTEM, ie. invoked as a service
    # Do whatever is necessary to stop the service script instance
    Write-EventLog -LogName $logName -Source $serviceName -EventId 1003 -EntryType Information -Message "$scriptName -Stop: Stopping script $scriptName -Service"
    Log "$scriptName -Stop: Stopping script $scriptName -Service"
    # Find the service script instance task and stop it.
    # Log (gwmi Win32_Process -filter "Name = 'powershell.exe'" | select ProcessId, CommandLine | ft -auto | Out-String -width 255)
    $scriptFullCname = $scriptFullName -replace "\\", "\\" # $scriptCopyCname is invalid when invoked as SYSTEM
    $processes = @(Get-WmiObject Win32_Process -filter "Name = 'powershell.exe'" | Get-WmiObject {
      $_.CommandLine -match ".*$scriptFullCname.*-Service"
    })
    foreach ($process in $processes) { # There should be just one, but be prepared for surprises.
      Log "$scriptName -Stop: Stopping PID $($process.ProcessId)"
      Stop-Process -Id $process.ProcessId -Force
    }
  } else {
    Write-Verbose "Stopping service $serviceName"
    Write-EventLog -LogName $logName -Source $serviceName -EventId 1004 -EntryType Information -Message "$scriptName -Stop: Stopping service $serviceName"
    Stop-Service $serviceName # Ask Service Control Manager to stop it
  }
  return
}

if ($Restart) {                 # Restart the service
  & $scriptFullName -Stop
  & $scriptFullName -Start
  return
}

if ($Status) {                  # Get the current service status
  $spid = $null
  $processes = @(Get-WmiObject Win32_Process -filter "Name = 'powershell.exe'" | Where-Object {
    $_.CommandLine -match ".*$scriptCopyCname.*-Service"
  })
  foreach ($process in $processes) { # There should be just one, but be prepared for surprises.
    $spid = $process.ProcessId
    Write-Verbose "$serviceName Process ID = $spid"
  }
  # if (Test-Path "HKLM:\SYSTEM\CurrentControlSet\services\$serviceName") {}
  try {
    $pss = Get-Service $serviceName -ea stop # Will error-out if not installed
  } catch {
    "Not Installed"
    return
  }
  $pss.Status
  if (($pss.Status -eq "Running") -and (!$spid)) { # This happened during the debugging phase
    Write-Error "The Service Control Manager thinks $serviceName is started, but $serviceName.ps1 -Service is not running."
    exit 1
  }
  return
}

if ($Setup) {                   # Install the service
  # Check if it's necessary
  try {
    $pss = Get-Service $serviceName -ea stop # Will error-out if not installed
    # Check if this script is newer than the installed copy.
    if ((Get-Item $scriptCopy -ea SilentlyContinue).LastWriteTime -lt (Get-Item $scriptFullName -ea SilentlyContinue).LastWriteTime) {
      Write-Verbose "Service $serviceName is already Installed, but requires upgrade"
      & $scriptFullName -Remove
      throw "continue"
    } else {
      Write-Verbose "Service $serviceName is already Installed, and up-to-date"
    }
    exit 0
  } catch {
    # This is the normal case here. Do not throw or write any error!
    Write-Debug "Installation is necessary" # Also avoids a ScriptAnalyzer warning
    # And continue with the installation.
  }
  if (!(Test-Path $installDir)) {
    New-Item -ItemType directory -Name $installDir | Out-Null
  }
  # Copy the service script into the installation directory
  if ($ScriptFullName -ne $scriptCopy) {
    Write-Verbose "Installing $scriptCopy"
    Copy-Item $ScriptFullName $scriptCopy
  }
  # Generate the service .EXE from the C# source embedded in this script
  try {
    Write-Verbose "Compiling $exeFullName"
    Add-Type -TypeDefinition $source -Language CSharp -OutputAssembly $exeFullName -OutputType ConsoleApplication -ReferencedAssemblies "System.ServiceProcess" -Debug:$false
  } catch {
    $msg = $_.Exception.Message
    Write-error "Failed to create the $exeFullName service stub. $msg"
    exit 1
  }
  # Register the service
  Write-Verbose "Registering service $serviceName"
  $pss = New-Service $serviceName $exeFullName -DisplayName $serviceDisplayName -StartupType Automatic

  return
}

if ($Remove) {                  # Uninstall the service
  # Check if it's necessary
  try {
    $pss = Get-Service $serviceName -ea stop # Will error-out if not installed
  } catch {
    Write-Verbose "Already uninstalled"
    return
  }
  Stop-Service $serviceName # Make sure it's stopped
  # In the absence of a Remove-Service applet, use sc.exe instead.
  Write-Verbose "Removing service $serviceName"
  $msg = sc.exe delete $serviceName
  if ($LastExitCode) {
    Write-Error "Failed to remove the service ${serviceName}: $msg"
    exit 1
  } else {
    Write-Verbose $msg
  }
  # Remove the installed files
  if (Test-Path $installDir) {
    foreach ($ext in ("exe", "pdb", "ps1")) {
      $file = "$installDir\$serviceName.$ext"
      if (Test-Path $file) {
        Write-Verbose "Deleting file $file"
        Remove-Item $file
      }
    }
    if (!(@(Get-ChildItem $installDir -ea SilentlyContinue)).Count) {
      Write-Verbose "Removing directory $installDir"
      Remove-Item $installDir
    }
  }
  return
}

if ($Service) {                 # Run the service
  Write-EventLog -LogName $logName -Source $serviceName -EventId 1005 -EntryType Information -Message "$scriptName -Service # Beginning background job"
  Log "$scriptName -Service # Beginning background job"
  # Do the service background job
  try {
    while ($true) { # Keep running until killed by the -Stop handler
      ######### TO DO: Implement your own service code here. ##########
      ###### Example that wakes up and logs a line every 10 sec: ######
      Start-Sleep 10
      Log "$script -Service # Awaken after 10s"
    }
  } catch { # An exception occurred while runnning the service
    $msg = $_.Exception.Message
    Log "$script -Service # Error: $msg"
  } finally { # Invoked in all cases: Exception or killing by -Stop
    # Record a termination event, no matter what the cause is.
    Write-EventLog -LogName $logName -Source $serviceName -EventId 1006 -EntryType Information -Message "$script -Service # Exiting"
    Log "$script -Service # Exiting"
  }
  return
}

