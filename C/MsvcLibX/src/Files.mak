###############################################################################
#									      #
#   File name	    Files.mak						      #
#									      #
#   Description     MsvcLibX Specific file dependancies			      #
#									      #
#   Notes	    							      #
#									      #
#   History								      #
#    2012-10-21 JFL Initial version                                           #
#    2013-03-27 JFL Added debugv.obj and getppid.obj.                         #
#    2014-02-03 JFL Added readlink.obj.                                       #
#    2014-02-05 JFL Added symlink.obj.                                        #
#    2014-02-06 JFL Added lstat*.obj.                                         #
#    2014-02-10 JFL Added realpath.obj.                                       #
#    2014-02-17 JFL Added err2errno.obj.                                      #
#    2014-02-26 JFL Added filetime.obj.                                       #
#    2014-02-27 JFL Added iconv.obj.                                          #
#    2014-02-28 JFL Added chdir.obj and getcwd.obj.                           #
#    2014-03-04 JFL Added fopen.obj.                                          #
#    2014-03-06 JFL Added strerror.obj.                                       #
#    2014-03-24 JFL Added access.obj.                                         #
#    2014-03-27 JFL Added spawn.obj.                                          #
#    2014-05-30 JFL Moved here the OBJECTS macro definition from NMakeFile.   #
#		    Added uname.obj and utimes.obj.                           #
#    2014-06-04 JFL Added clock_gettime.obj and gettimeofday.obj.             #
#    2014-06-24 JFL Added fstat64.obj and fstat64i32.obj.                     #
#    2014-07-01 JFL Added mb2wpath.obj.			                      #
#                   							      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #

# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #

###############################################################################

# List of object files to build and include in the MsvcLibX library
# IMPORTANT NOTE: Every time you add an object file in the list here, also
#                 store its specific source file dependancies below.
OBJECTS = \
    +access.obj        \
    +chdir.obj         \
    +clock_gettime.obj \
    +debugv.obj        \
    +dirent.obj        \
    +err2errno.obj     \
    +filetime.obj      \
    +fnmatch.obj       \
    +fopen.obj         \
    +fstat64i32.obj    \
    +fstat64.obj       \
    +getcwd.obj        \
    +getopt.obj        \
    +getppid.obj       \
    +gettimeofday.obj  \
    +iconv.obj         \
    +lstat64i32.obj    \
    +lstat64.obj       \
    +main.obj          \
    +mb2wpath.obj      \
    +mkdir.obj         \
    +mkdtemp.obj       \
    +mkstemp.obj       \
    +readlink.obj      \
    +realpath.obj      \
    +spawn.obj         \
    +strerror.obj      \
    +strndup.obj       \
    +strptime.obj      \
    +symlink.obj       \
    +uname.obj         \
    +utime.obj         \
    +utimes.obj        \
    +xfreopen.obj      \
#    +lstat32.obj       \
#    +lstat32i64.obj    \


# GnuLib routines that I mistakenly defined here
REMOVED_OBJECTS = \
    +error.obj      \
    +initmain.obj   \
    +xnmalloc.obj   \

###############################################################################
#			Include files dependancies			      #
###############################################################################

I=..\include

$(I)\chdir.h: $(I)\unistd.h $(I)\iconv.h $(I)\debugm.h

$(I)\config.h: $(I)\msvclibx.h $(I)\stdbool.h $(I)\unistd.h

$(I)\direct.h: $(I)\msvclibx.h 

$(I)\dirent.h: $(I)\inttypes.h $(I)\sys\stat.h

$(I)\error.h: $(I)\msvclibx.h 

# $(I)\fadvise.h:  

$(I)\fnmatch.h: $(I)\msvclibx.h 

$(I)\getcwd.h: $(I)\unistd.h $(I)\debugm.h

# $(I)\getopt.h: 

$(I)\grp.h: $(I)\msvclibx.h 

# $(I)\inttypes.h: 

# $(I)\msvclibx.h: 

# $(I)\netdb.h: 

$(I)\process.h: $(I)\msvclibx.h 

$(I)\pwd.h: $(I)\msvclibx.h 

# $(I)\regex.h: 

$(I)\sys\stat.h: $(I)\msvclibx.h $(I)\sys\types.h

# $(I)\stdbool.h: 

# $(I)\stdint.h: 

# $(I)\stdio--.h: 

# $(I)\system.h: 

$(I)\unistd.h: $(I)\msvclibx.h $(I)\dirent.h

# $(I)\utime.h:  

$(I)\xfreopen.h: $(I)\msvclibx.h 

$(I)\sys\types.h: $(I)\msvclibx.h 


###############################################################################
#			Source files dependancies			      #
###############################################################################

access.c: $(I)\MsvcLibX.h $(I)\debugm.h

chdir.c: $(I)\debugm.h $(I)\iconv.h $(I)\unistd.h

clock_gettime.c: $(I)\MsvcLibX.h $(I)\time.h $(I)\sys\stat.h

debugv.c: $(I)\debugm.h

dirent.c: $(I)\debugm.h $(I)\dirent.h $(I)\sys\stat.h $(I)\unistd.h

err2errno.c: $(I)\MsvcLibX.h $(I)\debugm.h

error.c: $(I)\config.h $(I)\error.h

filetime.c: $(I)\sys\stat.h

fnmatch.c: $(I)\debugm.h $(I)\fnmatch.h

fopen.c: $(I)\MsvcLibX.h

fstat64.c: fstat.c $(I)\debugm.h $(I)\dirent.h $(I)\MsvcLibX.h $(I)\sys\stat.h $(I)\stdint.h

fstat64i32.c: fstat.c $(I)\debugm.h $(I)\dirent.h $(I)\MsvcLibX.h $(I)\sys\stat.h $(I)\stdint.h

getcwd.c: $(I)\debugm.h $(I)\unistd.h

getopt.c: $(I)\getopt.h

# getppid.c:

gettimeofday.c: $(I)\MsvcLibX.h $(I)\time.h $(I)\sys\time.h

grp.c: $(I)\grp.h 

iconv.c: $(I)\iconv.h

initmain.c: $(I)\config.h

lstat32.c: lstat.c $(I)\debugm.h $(I)\dirent.h $(I)\MsvcLibX.h $(I)\sys\stat.h $(I)\stdint.h $(I)\unistd.h

lstat32i64.c: lstat.c $(I)\debugm.h $(I)\dirent.h $(I)\MsvcLibX.h $(I)\sys\stat.h $(I)\stdint.h $(I)\unistd.h

lstat64.c: lstat.c $(I)\debugm.h $(I)\dirent.h $(I)\MsvcLibX.h $(I)\sys\stat.h $(I)\stdint.h $(I)\unistd.h

lstat64i32.c: lstat.c $(I)\debugm.h $(I)\dirent.h $(I)\MsvcLibX.h $(I)\sys\stat.h $(I)\stdint.h $(I)\unistd.h

main.c: $(I)\MsvcLibX.h

mb2wpath.c: $(I)\MsvcLibX.h $(I)\debugm.h

mkdir.c: $(I)\MsvcLibX.h $(I)\sys\stat.h

mkdtemp.c: $(I)\unistd.h

mkstemp.c: $(I)\unistd.h

pwd.c: $(I)\pwd.h 

readlink.c: $(I)\debugm.h $(I)\unistd.h $(I)\reparsept.h

realpath.c: $(I)\debugm.h $(I)\unistd.h

spawm.c: $(I)\debugm.h $(I)\MsvcLibX.h $(I)\process.h

strerror.c: $(I)\MsvcLibX.h

# strndup.c: 

# strptime.c:

symlink.c: $(I)\debugm.h $(I)\reparsept.h $(I)\unistd.h

uname.c: $(I)\MsvcLibX.h $(I)\sys\utsname.h

utime.c: $(I)\debugm.h $(I)\unistd.h $(I)\utime.h $(I)\sys\time.h

xfreopen.c: $(I)\xfreopen.h

xnmalloc.c: $(I)\config.h


