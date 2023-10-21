/**********************************************************************/
/*                                                                    */
/*  PRODUCT      : Meta-Abstract Device Subsystem                     */
/*  COPYRIGHT    : (c) 2005 HTF CONSULTING                            */
/*                                                                    */
/**********************************************************************/
/*                                                                    */
/*  Exe file ID  : MAD_Monitor.exe, Functional driver Power functions */
/*                                                                    */
/*  Module  NAME : PowerNotify.h                                      */
/*  DESCRIPTION  : Header file with macros, defines &                 */
/*                 function prototypes                                */
/*                                                                    */
/**********************************************************************/
/*   Includes - Definitions - File Structures - Declarations          */
/*                                                                    */
/**********************************************************************/

#define POWERAXNFILE     "C:\\PMnotice.dat"
#define POWERAXNFILE_WC   L"\\??\\C:\\PMnotice.dat"

typedef struct
        {
        ULONG /*short int*/ PowerNotices[MAD_MAX_DEVICES];
        } POWERNOTIFY_TYPE;

#define INIT_POWERAXN {0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}









