Posix Shell scripts
===================

This directory contains Posix shell scripts.  

Most of the scripts here were actually initially written for Bash, then eventually generalized to run in any Posix shell.  
When adding new scripts, please avoid using any advanced shell feature (from bash, csh, zsh, etc),
that would prevent them to run in other such shells, or older basic Posix shells.  
If this cannot be avoided, please make sure that the #! shebang reflects that requirement.

The profile.d subdirectory contains initialization scripts that `make install` installs into /etc/profile.d/.
For portability to any Unix system, these scripts MUST be stricly Posix scripts.
