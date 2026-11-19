/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2018 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by Dual/GPL license to the Linux open source   */ 
/* community                                                                   */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe files   : madsimui.exe                                                 */ 
/*                                                                             */
/*  Module NAME : madsimui.cpp                                                 */
/*                                                                             */
/*  DESCRIPTION : Main module for the MAD simulation UI app                    */
/*                                                                             */
/*  MODULE_AUTHOR("HTF Consulting");                                           */
/*  MODULE_LICENSE("Dual/GPL");                                                */
/*                                                                             */
/* The source code in this file can be freely used, adapted, and redistributed */
/* in source or binary form, so long as an acknowledgment appears in derived   */
/* source files.  The citation should state that the source code comes from a  */
/* a set of source files developed by HTF Consulting                           */
/* http://www.htfconsulting.com                                                */
/*                                                                             */
/* No warranty is attached.                                                    */
/* HTF Consulting assumes no responsibility for errors or fitness of use       */
/*                                                                             */
/*                                                                             */
/* $Id: madsimui.cpp, v 1.0 2018/01/01 00:00:00 htf $                          */
/*                                                                             */
/*******************************************************************************/

//
#define _MAIN_
#include "madsimui.h"
//#include "madlib.h"

int Build_DevName_Open(char* DevName, int devno, int devnumdx, char* DevPathName, int* fd)

{
    strcpy(DevPathName, LINUX_DEVICE_PATH_PREFIX);
    DevName[devnumdx] = DEVNUMSTR[devno];
    strcat(DevPathName, DevName);
    //fprintf(stderr, "Opening device: %s\n", DevPathName);
    *fd = open(DevPathName, O_RDWR, 0);
    if (*fd < 1)
        {
 	    fprintf(stderr, "device open failure: errno=%d, devname=%s\n", errno, DevPathName);
 	    return errno;
        }

    return 0;
}

char MadBusObjName[] = MADBUSOBJNAME;
char MadBusObjPathName[100] = "";

PMADREGS pMadDevRegs = NULL;
u8*      pDevData = NULL;
int devnum = 0;

int main(int argc, char **argv)
{
//register int j;
bool bRC;

int rc = 0;
int fd = 0;
int op, parm, val;

    if (argc < 3)
        {   
    	display_help();
    	exit(0);
        }

    bRC = Parse_Cmd(argc, argv, &devnum, &op, &parm, &val);
    if (!bRC)
        {exit(0);}

    rc = Build_DevName_Open(MadBusObjName, devnum, MBDEVNUMDX, MadBusObjPathName, &fd);
    if (fd < 1)
        {
        rc = -errno;
     	goto exitmain;
        }

    rc = MapWholeDevice(&pMadDevRegs, fd);
    if (rc != 0)
        //{{fprintf(stderr, "MapDeviceRegs returned (%d)\n", rc);}
        {;}
    else
        {
    	pDevData = (u8*)((u64)pMadDevRegs + MAD_DEVICE_DATA_OFFSET);
        //DisplayDevRegs(pMadDevRegs);
        }
 
    rc = Process_Cmd(fd, op, parm, val);

    if (pMadDevRegs != NULL)
        if (pMadDevRegs != (void *)-1)
            munmap(pMadDevRegs, MAD_SAFE_MMAP_SIZE);
    
    close(fd);

    if (rc != 0)
        {fprintf(stderr, "madsimui exit... rc=%d\n", rc);}

exitmain:;
    return rc;
}

bool Parse_Cmd(int argc, char **argv,
		       int* devnum, int* op, int* parm, int* val)
{
    *devnum = atoi(argv[1]);
    if (*devnum == 0)
	{
	display_error_w_help(argv[1]);
	return false;
	}

    *op = kNOP;
    if (memcmp(argv[2], "nop", 3) == 0)
	{
        *val = 60;
        if (argv[3] != NULL)
            {
        	if (atoi(argv[3]) != 0)
                {*val = atoi(argv[3]);}
            }

        return true; //We permit open::close
	}

    if (memcmp(argv[2], "ini", 3) == 0)
        {*op = kINI;}

    if (memcmp(argv[2], "rst", 3) == 0)
        {*op = kRST;}

    if (memcmp(argv[2], "exp", 3) == 0)
        {*op = kEXP;}

    if (memcmp(argv[2], "idd", 3) == 0)
        {*op = kIDD;}

    if (memcmp(argv[2], "sav", 3) == 0)
        {*op = kSAV;}

    if (memcmp(argv[2], "res", 3) == 0)
        {*op = kRES;}

    if (memcmp(argv[2], "mget", 3) == 0)
        {*op = kMGET;}

    if (memcmp(argv[2], "get",3) == 0)
        {*op = kGET;}

    if (memcmp(argv[2], "set", 3) == 0)
        {*op = kSET;}

    if (memcmp(argv[2], "cbr", 3) == 0)
        {*op = kCBR;}

    if (memcmp(argv[2], "cbw", 3) == 0)
        {*op = kCBW;}

    if (memcmp(argv[2], "lrc", 3) == 0)
        {*op = kLRC;}

    if (memcmp(argv[2], "fwc", 3) == 0)
        {*op = kFWC;}

    if (memcmp(argv[2], "arc", 3) == 0)
        {*op = kARC;}

    if (memcmp(argv[2], "awc", 3) == 0)
        {*op = kAWC;}

    if (memcmp(argv[2], "hpl", 3) == 0)
        {*op = kHPL;}

    if (memcmp(argv[2], "hun", 3) == 0)
        {*op = kHUN;}

    if (*op == kNOP)
	{
	display_error_w_help(argv[2]);
	return false;
	}

    switch(*op)
	{
	case kINI:
	case kRST:
	case kMAP:
	case kEXP:
        case kGET:
        case kMGET:
	case kCBR:
	case kCBW:
	case kLRC:
	case kFWC:
	case kARC:
	case kAWC:
	    break;

       case kIDD:
   	    if (argv[3] != NULL)
                {*parm = (int)(*argv[3]);}
	    break;

        case kHPL:
            if (argv[3] == NULL)
    	        {
	    	display_error_w_help((char *)"*** enter a pci device id in hex # ***");
	    	return false;
	        }

	    *val = ATOH(argv[3]);
	    if (*val < 1)
	        {
	        display_error_w_help(argv[3]);
	        return false;
	        }
            //fall through

        case kHUN:
            *parm = *devnum;
            *devnum = 0;
            break;

        case kSAV:
        case kRES:
            *val = MAD_DEVICE_DATA_SIZE;
            if (argv[3] != NULL)
                if (atoi(argv[3]) != 0)
                    {*val = atoi(argv[3]);}
            break;

        case kSET:
	    *val = ATOH(argv[4]);
	    if (*val < 1)
	    	{
	    	display_error_w_help(argv[4]);
	    	return false;
	    	}
            //
	    *parm = kNOP;
	    //
	    if (memcmp(argv[3], "msi", 3) == 0)
   	    	{
	    	*parm = kMSI;
	      	break;
   	    	}
	    	//
	    if (memcmp(argv[3], "sts", 3) == 0)
   	    	{
	    	*parm = kSTS;
	    	break;
   	    	}
	    	//
	   if (*parm == kNOP)
	    	{
	   	display_error_w_help(argv[3]);
	   	return false;
	        }
	    	//
    	    break;

        default:
	    assert(false);
	    return false;
	}

    return true;
}


void display_error_w_help(char* pChar)
{
    printf("???  %s  ???\n", pChar);
    display_help();

    return;
}


void display_help()

{
    printf("\nmadsimui {No args} : display help\n");
    printf(" madsimui #  op    parms-description... \n");
    printf("Command examples... \n");
    printf(" madsimui 1  nop   {sss} open device (1); pause sss/60 seconds\n");
    printf(" madsimui 2  hpl   {pci_dev_id} Hot PLug device 2 {PCI device_id} in hex\n");
    printf(" madsimui 2  hun   Hot UNplug device x\n");
    printf(" madsimui 1  ini   INItialize device 1\n");
    printf(" madsimui 2  idd   {b/s} Initialize Device(2) Data... bytes / sectors(dflt)\n");
    printf(" madsimui x  sav   SAVe device data to file: maddevx.dat\n");
    printf(" madsimui x  res   REStore device data from file: maddevx.dat\n");
    printf(" madsimui 2  exp   EXPire (play dead) device 2\n");
    printf(" madsimui 3  get   GET device state 3\n");
    printf(" madsimui 3  mget  GET device state through Mmap() from device 3\n");
    printf(" madsimui 3  set   {reg} <value> SET register value on device 3\n");
    printf("                   registers: {msi sts iid}; all values in hexadecimal\n");
    //printf("I/O completions... \n");
    //printf(" madsimui x  cbr   Complete Buffered Read to device x\n");
    //printf(" madsimui x  cbw   Complete Buffered Write to device x\n");
    //printf(" madsimui x  lrc   Load Read Cache on device x\n");
    //printf(" madsimui x  fwc   Flush Write Cache on device x\n");
    //printf(" madsimui x  arc   Align Read Cache on device x\n");
    //printf(" madsimui x  awc   Align Write Cache on device x\n");
    printf("\n");

    return;
}

int Process_Cmd(int fd, int op, int parm, int val)

{
int rc = 0;
MADBUSCTLPARMS MadBusCtlParms;

    switch (op)
	{
        case kNOP:
    	    fprintf(stderr, "Process %d waiting %d seconds before terminating\n", getpid(), val);
    	    sleep(val);
            break;

	case kINI: //Init, ???
	case kRST:
	    rc = ioctl(fd, MADBUS_IOCTL_RESET, NULL);
	    break;

        case kHPL:
            MadBusCtlParms.Parm = parm; //devnum (slotnum) 
            MadBusCtlParms.Val  = val;  //pci_devid
		    rc = ioctl(fd, MADBUS_IOCTL_HOT_PLUG, &MadBusCtlParms);
			break;
 
	    case kHUN:
		    rc = ioctl(fd, MADBUS_IOCTL_HOT_UNPLUG, parm);
			break;

	    case kMGET:
   	        rc = (pMadDevRegs != NULL) ? 
   	              0 : MapDeviceRegs(&pMadDevRegs, fd);
   	        if (rc == 0)
   	            DisplayDevRegs(pMadDevRegs);
	        break;

	    case kEXP: //Play dead whole device
		rc = ioctl(fd, MADBUS_IOCTL_EXPIRE, NULL);
		break;

	    case kIDD:
		rc = Init_Device_Data(parm, pDevData);
               break;

	    case kSAV:
	    	rc = Save_Device_Data(pDevData, (char *)MAD_DATA_FILE_PREFIX, devnum, val);
	    	break;

	    case kRES:
	    	rc = Load_Device_Data(pDevData, (char *)MAD_DATA_FILE_PREFIX, devnum, val);
	    	break;

	    case kGET: //The whole device
		 rc = ioctl(fd, MADBUS_IOCTL_GET_DEVICE, &MadBusCtlParms);
		if (rc == 0)
		    {DisplayDevRegs(&MadBusCtlParms.MadRegs);}
		break;

	    case kSET:
		if (parm == kMSI)
                    pMadDevRegs->MesgID = val;

		if (parm == kSTS)
		    pMadDevRegs->Status = val;

		if (parm == kIID)
                    pMadDevRegs->IntID = val;
		break;

	    case kCBR:
	    	pMadDevRegs->MesgID = 1;
	    	break;

	    case kCBW:
	    	pMadDevRegs->MesgID = 2;
	    	break;

	    case kLRC:
	    	pMadDevRegs->MesgID = 3;
	    	break;

	    case kFWC:
	    	pMadDevRegs->MesgID = 4;
	    	break;

	    case kARC:
	    	pMadDevRegs->MesgID = 5;
	    	break;

	    case kAWC:
	    	pMadDevRegs->MesgID = 6;
	    	break;

	    default:
		assert(false);
		rc = -EINVAL;
		}

    if (rc == -1)
	rc = -errno;

    if (rc != 0)
      fprintf(stderr, "Process_Cmd rc=%d\n", rc);

    return rc;
}


int Init_Device_Data(int b_s, u8* pDevData)
{
    U32 j;
    //
    static char    SrcData[] = DEVNUMSTR;
    static size_t  DevLen = (MAD_SAFE_MMAP_SIZE - MAD_DEVICE_DATA_OFFSET); 
    //
    size_t fill_len = MAD_UNITIO_SIZE_BYTES;
    u32   srcdx = 0;
    u8*   pData;
    int   rc = 0;
    u32   NumSegs;
    u8    fillchar = '.';

    if (pDevData == NULL)
        {return -EINVAL;}

    memset(pDevData, fillchar, DevLen);
    return rc;

    if  ((char)b_s == 's') //bytes vs sectors
        {fill_len = MAD_SECTOR_SIZE;}

    NumSegs = DevLen / fill_len;
    pData   = pDevData;

    for (j=0; j < NumSegs; j++)
	{
	//fprintf(stderr, "%d ", j);
        srcdx = (j % strlen(SrcData));
        fillchar = '.'; // SrcData[srcdx];
        memset(pData, fillchar, fill_len);
        pData += fill_len;
        //sleep(.25);
	}

    return rc;
}

int Save_Device_Data(u8* pDevData, char* file_prefx, int devnum, size_t dataLen)
{
    return Xfer_Device_Data(pDevData, file_prefx, devnum, dataLen, 1);
}
//
int Load_Device_Data(u8* pDevData, char* file_prefx, int devnum, size_t dataLen)
{
    return Xfer_Device_Data(pDevData, file_prefx, devnum, dataLen, 0);
}
//
int Xfer_Device_Data(u8* pDevData, char* file_prefx, int devnum, size_t DataLen, u8 bWrite)
{
    static char digits[] = DEVNUMSTR;
    //
    int indx = strlen(file_prefx);
    U32 OpenFlags = (bWrite) ? (O_CREAT | O_WRONLY) : O_RDONLY;
    //
    int rc = 0;
    int fd;
    ssize_t iocount;
    char FileName[100] = "";

    if (pDevData == NULL)
        {return -ENOSYS;}
        
    strcpy(FileName, file_prefx);
    FileName[indx] = digits[devnum];
    strcat(FileName, ".dat");
    fd = open(FileName, OpenFlags, 0);
    if (fd < 1)
        {
 	 fprintf(stderr, "File open failure: errno=%d file name=%s\n", errno, FileName);
 	 return errno;
        }
    
    iocount = 
    (bWrite) ? write(fd, pDevData, DataLen) : read(fd, pDevData, DataLen);
    if(iocount < 0)
        {rc = (int)iocount;}
    else
        {fprintf(stderr, "I/O complete to file: (%s) %ld bytes transferred from %px\n",
                 FileName, iocount, pDevData);}
 
    return rc;
}

