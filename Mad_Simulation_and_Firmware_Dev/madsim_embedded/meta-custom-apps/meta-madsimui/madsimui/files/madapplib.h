/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2021 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by Dual/GPL license to the Linux open source   */ 
/* community                                                                   */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe files   : madsimui.exe, madtest.exe                                    */ 
/*                                                                             */
/*  Module NAME : madapplib.h                                                  */
/*                                                                             */
/*  DESCRIPTION : Function prototypes and definitions for the MAD app exes     */
/*                                                                             */
/*  MODULE_AUTHOR("HTF Consulting");                                           */
/*  MODULE_LICENSE("Dual/GPL");                                                */
/*                                                                             */
/* The source code in this file can be freely used, adapted, and redistributed */
/* in source or binary form, so long as an acknowledgment appears in derived   */
/* source files.  The citation should state that the source code comes from    */
/* a set of source files developed by HTF Consulting                           */
/* http://www.htfconsulting.com                                                */
/*                                                                             */
/* No warranty is attached.                                                    */
/* HTF Consulting assumes no responsibility for errors or fitness of use       */
/*                                                                             */
/*                                                                             */
/* $Id: madapplib.h, v 1.0 2021/01/01 00:00:00 htf $                           */
/*                                                                             */
/*******************************************************************************/

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

int ATOH(char* hexstr);
int Build_DevName_Open(char* DevName, 
                       int devno, int devnumdx, char* DevPathName, int* fd);
int MapDeviceSection(void **ppSection, int fd, size_t MapLen, ULONG DevOfset);
int MapWholeDevice(PMADREGS *ppmadregs, int fd);
int MapDeviceRegsPio(PMADREGS *ppmadregs, int fd);
int MapDeviceRegs(PMADREGS *ppmadregs, int fd);

int MapDeviceIoOfset(void **ppIoOfset, int write, int fd);
void DisplayDevRegs(PMADREGS pmadregs);

//Function definitions
//
#ifdef _MAIN_ 
//
int ATOH(char* hexstr)
{
int j;
//
static char HexDigits[] = "0123456789abcdef";
//
char* pc;
int val = 0;
int indx = 0;

    int len = strlen(hexstr);
    for (j=0; j < len; j++)
	{
	pc = strchr(HexDigits, hexstr[j]);
	if (pc == NULL)
	   return -1;

	indx = (long int)pc - (long int)HexDigits;
	val = (val << 4);
	val += indx;
	}

    return val;
}

int Build_DevName_Open(char* DevName, int devno, int devnumdx, int openflags, char* DevPathName, int* fd)

{
int rc=0;

    strcpy(DevPathName, LINUX_DEVICE_PATH_PREFIX);
    DevName[devnumdx] = DEVNUMSTR[devno];
    strcat(DevPathName, DevName);
    //fprintf(stderr, "Opening device: %s\n", DevPathName);
    *fd = open(DevPathName, (O_RDWR|openflags), 0);
    if (*fd < 1)
        {
        rc =- errno;
 	fprintf(stderr, "device open failure: errno=%d, devname=%s\n", rc, DevPathName);
 	//return rc;
        }

    return rc;
}
//
int MapDeviceSection(void **ppSection, int fd, size_t maplen, ULONG DevOfset)
{
    static ULONG MapFlags = MAP_SHARED_VALIDATE; 
    size_t MapLen = MIN(maplen, MAD_SAFE_MMAP_SIZE); 
    int rc = 0;

    *ppSection = 
    mmap(NULL, MapLen, (PROT_READ|PROT_WRITE), MapFlags, fd, DevOfset);
    if (*ppSection == NULL)
	{rc = -EPERM;}
    else
	{
	if ((long int)(*ppSection) == -1)
	    {
            *ppSection = NULL;
	    rc = -errno;
	    }
	//else
	    //{fprintf(stderr, "MapDeviceSection: mmap returns umaddr=%p\n", *ppSection);}
 	}

    if (rc != 0)
	{fprintf(stderr, "MapDeviceSection: mmap returns rc=%d\n", rc);}

    return rc;
}
//
int MapWholeDevice(PMADREGS *ppmadregs, int fd)
{
    static ULONG DevRegsOfset = 0;

    return MapDeviceSection((void **)ppmadregs, fd, MAD_DEVICE_MAP_MEM_SIZE, DevRegsOfset);
}
//
int MapDeviceRegsPio(PMADREGS *ppmadregs, int fd)
{
    static ULONG DevRegsOfset = 0;

    return MapDeviceSection((void **)ppmadregs, fd, MAD_DEVICE_MEM_SIZE_NODATA, DevRegsOfset);
}
//
int MapDeviceRegs(PMADREGS *ppmadregs, int fd)
{
    static ULONG DevRegsOfset = 0;

    return MapDeviceSection((void **)ppmadregs, fd, MAD_REGISTER_BLOCK_SIZE, DevRegsOfset);
}
//
int MapDeviceIoOfset(void **ppIoOfset, int write, int fd)
{
    ULONG  Ofset  = (write) ? MAD_MAPD_WRITE_OFFSET : MAD_MAPD_READ_OFFSET;
    size_t IoSize = (write) ? MAD_MAPD_WRITE_SIZE : MAD_MAPD_READ_SIZE;

    return MapDeviceSection((void **)ppIoOfset, fd, IoSize, Ofset);
}
//
void DisplayDevRegs(PMADREGS pmadregs)
{
    fprintf(stderr, "Current device registers at: %p\n", pmadregs);
    fprintf(stderr, "Devnum=%d MesgID=x%X Control=x%X Status=x%X IntEnable=x%X IntID=x%X\n",
  	    (int)pmadregs->Devnum, (unsigned int)pmadregs->MesgID,
  	    (unsigned int)pmadregs->Control, (unsigned int)pmadregs->Status,
	    (unsigned int)pmadregs->IntEnable, (unsigned int)pmadregs->IntID);
    fprintf(stderr, "ReadDX=x%X  WriteDX=x%X  ReadCacheDx=x%X WriteCacheDx=x%X PwrState=x%X\n",
    	    (unsigned int)pmadregs->ByteIndxRd, (unsigned int)pmadregs->ByteIndxWr,
	    (unsigned int)pmadregs->CacheIndxRd, (unsigned int)pmadregs->CacheIndxWr,
	    (unsigned int)pmadregs->PowerState);

    return;
}
//
#endif  //_MAIN_

