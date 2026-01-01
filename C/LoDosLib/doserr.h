/************************ :encoding=UTF-8:tabSize=8: *************************\
*                                                                             *
*   Filename	    doserr.h						      *
*									      *
*   Description     MS-DOS error codes					      *
*                                                                             *
*   Notes	    These constants are defined in the MS-DOS 5 Programmer's  *
*		    Reference, in appendix C.				      *
*		    They're also defined in the Windows 98 DDK's error.h      *
*		    include file, but curiously not in any include file of    *
*		    the MASM or MSVC compilers for DOS.			      *
*		    							      *
*		    These errors are returned in AX by all DOS 2.0+ functions *
*		    that set the carry flag in case of error.		      *
*		    If the carry is set, the intdos() and intdosx() functions *
*		    save that error into the global variable _doserrno.	      *
*		    Undocumented: These two routines also convert that DOS    *
*		    error code into a standard C error code, stored in the    *
*		    global variable errno.				      *
*		    							      *
*   History								      *
*    2025-11-25 JFL Created this file.					      *
*									      *
*                   © Copyright 2025 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _DOSERR_H_
#define _DOSERR_H_

#define ERROR_INVALID_FUNCTION		0x0001	/*   1 - The function number is invalid              */
#define ERROR_FILE_NOT_FOUND		0x0002	/*   2 - The file could not be found                 */
#define ERROR_PATH_NOT_FOUND		0x0003	/*   3 - The path could not be found                 */
#define ERROR_TOO_MANY_OPEN_FILES	0x0004	/*   4 - There are too many files open simultaneously*/
#define ERROR_ACCESS_DENIED		0x0005	/*   5 - Access is denied                            */
#define ERROR_INVALID_HANDLE		0x0006	/*   6 - The handle identifier is unknown            */
#define ERROR_ARENA_TRASHED		0x0007	/*   7 - The memory control block is destroyed       */
#define ERROR_NOT_ENOUGH_MEMORY		0x0008	/*   8 - There is insufficient available memory      */
#define ERROR_INVALID_BLOCK		0x0009	/*   9 - The memory address is incorrect             */
#define ERROR_BAD_ENVIRONMENT		0x000A	/*  10 - The environment is incorrect                */
#define ERROR_BAD_FORMAT		0x000B	/*  11 - The format is invalid                       */
#define ERROR_INVALID_ACCESS		0x000C	/*  12 - The access code is incorrect                */
#define ERROR_INVALID_DATA		0x000D	/*  13 - The data is incorrect                       */

#define ERROR_INVALID_DRIVE		0x000F	/*  15 - The drive is unknown                                         */
#define ERROR_CURRENT_DIRECTORY		0x0010	/*  16 - An attempt was made to destroy the current directory         */
#define ERROR_NOT_SAME_DEVICE		0x0011	/*  17 - The devices are different                                    */
#define ERROR_NO_MORE_FILES		0x0012	/*  18 - There are no more files in the list                          */
#define ERROR_WRITE_PROTECT		0x0013	/*  19 - The media is write-protected                                 */
#define ERROR_BAD_UNIT			0x0014	/*  20 - The device is unknown                                        */
#define ERROR_NOT_READY			0x0015	/*  21 - The device is not ready                                      */
#define ERROR_BAD_COMMAND		0x0016	/*  22 - The instruction is unknown                                   */
#define ERROR_CRC			0x0017	/*  23 - A CRC error was detected                                     */
#define ERROR_BAD_LENGTH		0x0018	/*  24 - The data width is incorrect                                  */
#define ERROR_SEEK			0x0019	/*  25 - The search was unsuccessful                                  */
#define ERROR_NOT_DOS_DISK		0x001A	/*  26 - The device type is unknown                                   */
#define ERROR_SECTOR_NOT_FOUND		0x001B	/*  27 - The sector could not be found                                */
#define ERROR_OUT_OF_PAPER		0x001C	/*  28 - The printer is out of paper                                  */
#define ERROR_WRITE_FAULT		0x001D	/*  29 - A write error occurred                                       */
#define ERROR_READ_FAULT		0x001E	/*  30 - An error occurred while reading                              */
#define ERROR_GEN_FAILURE		0x001F	/*  31 - This is a general class error                                */
#define ERROR_SHARING_VIOLATION		0x0020	/*  32 - The shared resource is already in use                        */
#define ERROR_LOCK_VIOLATION		0x0021	/*  33 - The shared resource is locked                                */
#define ERROR_WRONG_DISK		0x0022	/*  34 - An invalid disk swap occurred                                */
#define ERROR_FCB_UNAVAILABLE		0x0023	/*  35 - The FCB is unavailable                                       */
#define ERROR_SHARING_BUFFER_EXCEEDED	0x0024	/*  36 - The resource buffer overflow occurred                        */
#define ERROR_CODE_PAGE_MISMATCHED	0x0025	/*  37 - The character code page does not correspond                  */
#define ERROR_HANDLE_EOF		0x0026	/*  38 - The file operation could not be completed (not enough input) */
#define ERROR_HANDLE_DISK_FULL		0x0027	/*  39 - There is insufficient disk space                             */

#define ERROR_NOT_SUPPORTED		0x0032	/*  50 - The network call is not supported                  */
#define ERROR_REM_NOT_LIST		0x0033	/*  51 - The remote computer is not responding              */
#define ERROR_DUP_NAME			0x0034	/*  52 - There is a duplicate name on the network           */
#define ERROR_BAD_NETPATH		0x0035	/*  53 - The network name could not be found                */
#define ERROR_NETWORK_BUSY		0x0036	/*  54 - The network is busy                                */
#define ERROR_DEV_NOT_EXIST		0x0037	/*  55 - The network device does not exist                  */
#define ERROR_TOO_MANY_CMDS		0x0038	/*  56 - The network BIOS command is too large              */
#define ERROR_ADAP_HDW_ERR		0x0039	/*  57 - The network adapter hardware is causing problems   */
#define ERROR_BAD_NET_RESP		0x003A	/*  58 - The network response is invalid                    */
#define ERROR_UNEXP_NET_ERR		0x003B	/*  59 - There are unexpected problems from the network     */
#define ERROR_BAD_REM_ADAP		0x003C	/*  60 - The adapter is incompatible                        */
#define ERROR_PRINTQ_FULL		0x003D	/*  61 - The print queue is full                            */
#define ERROR_NO_SPOOL_SPACE		0x003E	/*  62 - The print queue is still full                      */
#define ERROR_PRINT_CANCELLED		0x003F	/*  63 - The print file was deleted                         */
#define ERROR_NETNAME_DELETED		0x0040	/*  64 - The network name was deleted                       */
#define ERROR_NET_ACCESS_DENIED		0x0041	/*  65 - Network access denied                              */
#define ERROR_BAD_DEV_TYPE		0x0042	/*  66 - The network device type is incorrect               */
#define ERROR_BAD_NET_NAME		0x0043	/*  67 - The network name could not be found                */
#define ERROR_TOO_MANY_NAMES		0x0044	/*  68 - The network name is too large                      */
#define ERROR_TOO_MANY_SESS		0x0045	/*  69 - The network BIOS session is too large              */
#define ERROR_SHARING_PAUSED		0x0046	/*  70 - The server share has stopped                       */
#define ERROR_REQ_NOT_ACCEP		0x0047	/*  71 - The server refused our request                     */
#define ERROR_REDIR_PAUSED		0x0048	/*  72 - The network printer redirection has stopped        */

#define ERROR_FILE_EXISTS		0x0050	/*  80 - The file already exists                                */
#define ERROR_DUP_FCB			0x0051	/*  81 - This is a duplicate FCB                                */
#define ERROR_CANNOT_MAKE		0x0052	/*  82 - It is not possible to create a directory               */
#define ERROR_FAIL_I24			0x0053	/*  83 - A problem caused a 24-hour outage (critical error)     */
#define ERROR_OUT_OF_STRUCTURES		0x0054	/*  84 - Out of structures, likely due to too many redirections */
#define ERROR_ALREADY_ASSIGNED		0x0055	/*  85 - A duplicate redirection                                */
#define ERROR_INVALID_PASSWORD		0x0056	/*  86 - An invalid password                                    */
#define ERROR_INVALID_PARAMETER		0x0057	/*  87 - The invalid parameter                                  */
#define ERROR_NET_WRITE_FAULT		0x0058	/*  88 - A write error occurred on a network device             */
#define ERROR_NO_PROC_SLOTS		0x0059  /*  89 - This function is not supported by the network          */
#define ERROR_SYS_COMP_NOT_LOADED	0x005A	/*  90 - The required system components are not installed       */

#endif /* _DOSERR_H_ */
