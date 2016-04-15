@echo off
:# runas /user:Administrator "%windir%\System32\notepad.exe" "%windir%\System32\Drivers\etc\hosts"
:# runas /user:%USERDOMAIN%\%USERNAME% "%windir%\System32\notepad.exe" "%windir%\System32\Drivers\etc\hosts"
elevate "%windir%\System32\notepad.exe" "%windir%\System32\Drivers\etc\hosts"
