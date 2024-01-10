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
/*  Exe files   : madtest.exe                                                  */ 
/*                                                                             */
/*  Module NAME : madtest.cpp                                                  */
/*                                                                             */
/*  DESCRIPTION : Main module for the MAD test app                             */
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
/* $Id: madtest.cpp, v 1.0 2021/01/01 00:00:00 htf $                           */
/*                                                                             */
/*******************************************************************************/

#define _MAIN_
#include "madtest.h"
#include <sys/stat.h>

char MadDevName[] = MADDEVNAME;
char MadDevPathName[100] = "";

MADREGS MadRegs;
PMADREGS pMapdDevRegs = NULL;

void* pPioregn = NULL;
void* pDevData = NULL;
char* pLargeBufr = NULL;
u8 RandomBufr[8192];

int main(int argc, char **argv)
{
//bool bQuiet = false;
int rc;
int fd = 1;
char* pBufr = NULL;
bool bRC;
int devnum = -1, op = -1;
long val = 0;
long offset = 0;
char parms[10];
void* parm = parms;
struct stat statstr;

    if (argc < 3)
        {
 	display_help();
 	exit(0);
        }

    bRC = Parse_Cmd(argc, argv, &devnum, &op, &val, &offset, &parm);
    if (!bRC)
        {
        fprintf(stderr, "Parse_Cmd failed! exit=9\n"); 
    	exit(9);
        }

    rc = Build_DevName_Open(MadDevName, devnum, MADDEVNUMDX, OPENFLAGS, MadDevPathName, &fd);
    if (fd < 1)
        {
        rc = -errno;
     	goto exitmain;
        }

    //rc = fstat(fd, &statstr);
    //fprintf(stderr, "ParseCmd:stat() fd=%d rc=%d err#=%d stmode=%d stsize=%ld\n",
    //        fd, rc, errno, statstr. st_mode, statstr.st_size);

    //rc = MapDeviceRegsPio(&pMapdDevRegs, fd);
    rc = MapWholeDevice(&pMapdDevRegs, fd);
    if (pMapdDevRegs != NULL)
        {
        pPioregn = ((U8*)pMapdDevRegs + MAD_MAPD_READ_OFFSET);
        pDevData = ((U8*)pMapdDevRegs + MAD_DEVICE_DATA_OFFSET);
	//fprintf(stderr, "madtest pRegs=%p pPIO=%p pData=%p\n",
	//        pMapdDevRegs, pPioregn, pDevData);
        }
        
    rc = Process_Cmd(fd, op, val, offset, parm);

    if (pBufr != NULL) //We have a buffer
        free(pBufr);
        
    if (pMapdDevRegs != NULL)
        if (pMapdDevRegs != (void *)-1)
            munmap(pMapdDevRegs, MAD_SAFE_MMAP_SIZE);

    close(fd);
      
    if (rc != 0)
	fprintf(stderr, "madtest returning rc=%d\n", rc);

exitmain:;
    exit(rc);
}


bool Parse_Cmd(int argc, char **argv,
		int* devnum, int* op, long* val, long *offset, void* *parm)
{
    long lparm;
    
    *devnum = atoi(argv[1]);
    if (*devnum == 0)
        {
	    display_error_w_help(argv[1]);
	    return false;
	    }

    *op = kNOP;
    if (memcmp(argv[2], "nop", 3) == 0)
	{
	if (argv[3] != NULL)
	    if (atoi(argv[3]) != 0)
	       *val = atoi(argv[3]);
	       
	return true; //We permit open::close
	}

    if (memcmp(argv[2], "map", 3) == 0)
        {*op = kMAP;}

    if (memcmp(argv[2], "ini", 3) == 0)
        {*op = kINI;}

    if (memcmp(argv[2], "rst", 3) == 0)
        {*op = kRST;}

    if (memcmp(argv[2], "get",3) == 0)
        {*op = kGET;}

    if (memcmp(argv[2], "mget",3) == 0)
        {*op = kMGT;}

    if (memcmp(argv[2], "set", 3) == 0)
        {*op = kSET;}

    if (strcmp(argv[2], "rb") == 0)
        {*op = kRB;}

    if (strcmp(argv[2], "wb") == 0)
        {*op = kWB;}

    if (memcmp(argv[2], "rba", 3) == 0)
	{*op = kRBA;}

    if (memcmp(argv[2], "wba", 3) == 0)
        {*op = kWBA;}

    if (memcmp(argv[2], "rbq", 3) == 0)
        {*op = kRBQ;}

    if (memcmp(argv[2], "wbq", 3) == 0)
        {*op = kWBQ;}
    
    if (memcmp(argv[2], "rdr", 3) == 0)
        {*op = kRDR;}

    if (memcmp(argv[2], "wrr", 3) == 0)
        {*op = kWRR;}

    if (memcmp(argv[2], "rdi", 3) == 0)
        {*op = kRDI;}

    if (memcmp(argv[2], "wdi", 3) == 0)
        {*op = kWDI;}

    if (memcmp(argv[2], "pir", 3) == 0)
        {*op = kPR;}

    if (memcmp(argv[2], "piw", 3) == 0)
        {*op = kPW;}

    if (memcmp(argv[2], "prc", 3) == 0)
        {*op = kPRC;}

    if (memcmp(argv[2], "pwc", 3) == 0)
        {*op = kPWC;}

    if (memcmp(argv[2], "arc", 3) == 0)
        {*op = kARC;}

    if (memcmp(argv[2], "awc", 3) == 0)
        {*op = kAWC;}


    if (*op == kNOP)
        {
	    display_error_w_help(argv[2]);
	    return false;
	    }

    switch (*op)
        {
	case kINI:
	case kRST:
        case kMAP:
        case kGET:
        case kMGT:
        case kPRC:
     	    break; //no more parameters - we're ready

    	case kPWC:
            if (argc < 4) //no write data entered
            	{
    	    	display_error_w_help(argv[4]);
    	    	return false;
            	}
           	*parm = argv[3];
    	    break;

        case kARC:
        case kAWC:
           *val = atoi(argv[3]); //iolen
     	    if (*val < 1)
     	        {
     	    	display_error_w_help(argv[3]);
     	    	return false;
     	        }
            break;

        case kPW:
      	    if (argc < 5) //no write data entered
         	{
 	    	display_error_w_help(argv[4]);
 	    	return false;
         	}
         *parm = argv[4];
         // fall through
         case kPR:
             *val = atoi(argv[3]); //iolen
 	      if (*val < 1)
 	    	  {
 	    	  display_error_w_help(argv[3]);
 	    	  return false;
 	    	 }
             break;

        case kWB:
        case kWBA:
        case kWBQ:
        case kWDI:
      	    if (argc < 6) //no write data entered
        	{
	    	display_error_w_help((char*)" No write data entered! ");
	    	return false;
        	}

            *parm = argv[5]; //The text-data from the command line
            // fall through
        case kRB:
        case kRBA:
        case kRBQ:
        case kRDI:
            *val = atoi(argv[3]); //iolen
	         if (*val < 1)
	         {
	    	 display_error_w_help(argv[3]);
	    	 return false;
	    	 }

             if ((*op == kRDI) || (*op == kWDI))
                 {
                 if (pLargeBufr == NULL)
                     {pLargeBufr = 
                     (char *)memalign(PAGE_SIZE, (*val * LINUX_PAGE_SIZE));} //checked during process_cmd
                 }
                 
            if (argc > 4)
                *offset = atoi(argv[4]); //offset
                 
            break;

        case kRDR:
        case kWRR:
            *val = atol(argv[3]);
            if (0 == (int)*val)
                {             
	    	 display_error_w_help(argv[3]);
	    	 return false;
	         } 
	         
	     if (pLargeBufr == NULL)
                 {pLargeBufr = 
                 (char *)memalign(PAGE_SIZE, (4 * LINUX_PAGE_SIZE));} 
 	     
            *offset = atol(argv[4]);
	        
	    if (*op == kRDR) 
	        return true;   
	           
	    if (argv[5] == NULL)
	        {             
	    	display_error_w_help(argv[5]);
	    	return false;
	        }
	            
            memcpy((char *)*parm, argv[5], strlen(argv[5]));
            break;
             
        case kSET:
	    //The set ioctls need a value
	    //
	    *val = ATOH(argv[4]);
	    if (*val < 1)
	    	{
	    	display_error_w_help(argv[4]);
	    	return false;
	    	}
	    //
	    *parm = (void *)kNOP;
	    //
	    if (memcmp(argv[3], "ien", 3) == 0)
                {*parm = (void *)kIEN;}

	    if (memcmp(argv[3], "ctl", 3) == 0)
                {*parm = (void *)kCTL;}
	    	//
	    if (*parm == (void *)kNOP)
	    	{
	    	display_error_w_help(argv[3]);
	    	return false;
	        }
	    	//
    	    break;

        default:
	    assert(false);
	    display_error_w_help(argv[2]);
	    return false;
	}

    return true;
}

void display_error_w_help(char* pErrParm)
{
	printf("???  %s  ???\n", pErrParm);
	display_help();

	return;
}

void display_help()

{
    printf("\n madtest {No args} : display help\n");
    printf("madtest #  op    params-description... \n");
    printf("Command examples... \n");
    printf(" madtest 1  nop   (sss) open device (1); pause sss/60 seconds\n");
    //printf(" madtest 2  map   get a user mode MAPping to device 2 through mmap()\n");
    printf(" madtest 2  ini   INIt device 2 \n");
    printf(" madtest 3  rst   ReSeT all data indeces on device 3\n");
    printf(" madtest 3  get   GET registers through an ioctl from device 3\n");
    printf(" madtest 3  mget  GET registers through Mmap() from device 3\n");
    printf(" madtest 3  set   {reg} <value> SET register value on device 3\n");
    printf("                  registers: {ien ctl}; all values in hexadecimal\n");
    
    printf("I/O operations...\n");
    printf(" madtest x  rb    <size>... Read <size> bytes bufr'd from device x\n");
    printf(" madtest x  wb    <size> {data}... Write <size> bytes {data} bufr'd to device x\n");
    //printf(" madtest x  rba   <size>... Read <size> bytes bufr'd from device x\n");
    //printf(" madtest x  wba   <size> {data}... Write <size> bytes {data} bufr'd to device x\n");
    printf(" madtest x  rbq   <size>... Read <size> bytes bufr'd from device x queued\n");
    printf(" madtest x  wbq   <size> {data}... Write <size> bytes {data} bufr'd to dev x q'd\n");
    printf(" madtest x  rdr   <size> <offset>... Read <size> pages from device x at <offset>n");
    printf(" madtest x  wrr   <size> <offset> {data}... Write <size> pages {data} at <offset> to dev x\n");
    //printf(" madtest x  rdi  <size> <offset>...  Read <size> pages Direct-IO from device x at <offset>\n");
    //printf(" madtest x  wdi  <size>  Write <size> pages Direct-IO to device x at <offset> {data}\n");
    //printf(" madtest x  pir   <size>... Read <size> bytes from device x Programmed Io regn\n");
    //printf(" madtest x  piw   <size> {data}... Write <size> bytes to device x PIO regn\n");
    printf(" madtest x  rd    <size> <offset>... Dma-Read <size> bytes from device x\n");
    printf(" madtest x  wd    <size> <offset>... Dma-Write <size> bytes to device x\n");
    //printf(" madtest x  arc   <sector>.... Align Read Cache on device x to <sector>\n");
    //printf(" madtest x  awc   <sector>.... Align Write Cache on device x to <sector>\n");
    //printf(" madtest x  prc   Pull (through) Read Cache <--- device x\n");
    //printf(" madtest x  pwc   Push (through) Write Cache ---> device x\n");
    printf("\n");
    return;
}

static u8 iobufr[MAD_CACHE_SIZE_BYTES];
int Process_Cmd(int fd, int op, long val, long offset, void* parm)

{
static u8 iobufr[MAD_CACHE_SIZE_BYTES];
//
ulong iparm = *(ULONG *)parm;
//
int rc = 0;
ssize_t iocount = 0;

ssize_t offsetr;
MADCTLPARMS MadCtlParms;
//void* pbufr;
char data[11];

    //fprintf(stderr, "Process_Cmd fd=%d op=%d val=%ld offset=%ld\n",
     //       fd, op, val, offset);
    //printf("Process_Cmd fd=%d op=%d val=%ld offset=%ld\n",
    //        fd, op, val, offset);

    switch (op)
	{
        case kNOP:
            fprintf(stderr, "Process %d terminating in %d seconds... \n", getpid(), val);
            sleep(val);
            break;

	case kINI:
	    rc = ioctl(fd, MADDEVOBJ_IOC_INIT, NULL);
	    break;

	case kRST:
	    rc = ioctl(fd, MADDEVOBJ_IOC_RESET, NULL);
	    if (pDevData != NULL)
	        {memset(pDevData, '.', 
	                (MAD_SAFE_MMAP_SIZE - MAD_DEVICE_DATA_OFFSET));}
	    break;
                                    
        case kRB:
        case kRBA:
        case kRBQ:
            //sleep(1);
    	    //memset(iobufr, 0x00, MAD_SECTOR_SIZE);
            if(op == kRB)
                {iocount = read(fd, iobufr, (size_t)val);}
            else
                {iocount = (op == kRBA) ? Async_Io(fd, iobufr, (size_t)val, 0, 0) :
                                         Queued_Io(fd, iobufr, (size_t)val, 0);}
            if (iocount < 0)
                {rc = (int)iocount;}
            else
                {fprintf(stderr, "Read buffered data returned: %d bytes...\n%s\n",
                                  (int)iocount, iobufr);}
      	    break;

        case kWB:
        case kWBA:
        case kWBQ:
            sleep(1);
            memset(iobufr, '.', MAD_SECTOR_SIZE);
            memcpy(iobufr, (char *)parm, strlen((char *)parm));

            if (op == kWB)
                {iocount = write(fd, iobufr, (size_t)val);}
            else
                {iocount = (op == kWBA) ? Async_Io(fd, iobufr, (size_t)val, 0, 1) :
                                          Queued_Io(fd, iobufr, (size_t)val, 1);}
            if (iocount < 0)
                {rc = iocount;}
            else
                {fprintf(stderr,
                         "Write buffered data completes: %d bytes\n",
                         (int)iocount);}
            break;
 
        /* Random read or write */
        case kWRR:
            memcpy(pLargeBufr, (char *)parm, strlen((char *)parm));
        case kRDR:
            {
            U8 bWrite = (op == kWRR);
            fprintf(stderr, "Random i/o bWr=%d size=%ld offset=%ld\n", bWrite, val, offset);

            /*rc = (op == kRDR) ? ioctl(fd, MADDEVOBJ_IOC_SET_READ_INDX, iparm) :
                                ioctl(fd, MADDEVOBJ_IOC_SET_WRITE_INDX, iparm);
	    if (rc != 0) 
	        {
	        fprintf(stderr, "ioctl set_RW_indx returned (%d)\n", rc);
	        break;
	        } */
 
            offsetr = lseek(fd, offset, SEEK_SET);
            //assert(offsetr == offset);
            fprintf(stderr, "fpos %ld set to %ld\n", offset, offsetr);
            sleep(1);
            iocount = (op == kRDR) ? read(fd, pLargeBufr, (size_t)val) :
                                     write(fd, pLargeBufr, (size_t)val); /* */
            if (iocount < 0)
                rc = (int)iocount;
            else
                if (op == kRDR)
                    {
                    pLargeBufr[100] = 0x00;
                    fprintf(stderr, "Data=%s\n", pLargeBufr);
                    }
            }
            break;
        
        case kRDI:
            if (pLargeBufr == NULL)
                {
                rc = -ENOMEM;
                break;
                }

            memset(pLargeBufr, '+', (LINUX_PAGE_SIZE * val));
            iocount = read(fd, pLargeBufr, (size_t)(LINUX_PAGE_SIZE * val));
            if (iocount < 0)
                {rc = (int)iocount;}
            else
                {
                pLargeBufr[1000] = 0x00;
                fprintf(stderr, "Read data direct-io returned: %d bytes...\n%s\n",
                        (int)iocount, (char *)pLargeBufr);
                memset(data, 0x00, 11);
                memcpy(data, pLargeBufr, 10);         
                fprintf(stderr, "x%X.%X.%X.%X.%X.%X.%X.%X.%X.%X\n",
                         data[0], data[1], data[2], data[3], data[4], 
                         data[5], data[6], data[7], data[8], data[9], data[10]);
                }
           	break;

        case kWDI:
            if (pLargeBufr == NULL)
                {
                rc = -ENOMEM;
                break;
                }

            memset(pLargeBufr, 0x00, (LINUX_PAGE_SIZE * val));
            memcpy(pLargeBufr, (char *)parm, strlen((char *)parm));
            iocount = write(fd, pLargeBufr, (size_t)(LINUX_PAGE_SIZE * val));
            if (iocount < 0)
                {rc = iocount;}
            else
                {fprintf(stderr,
                         "Write data direct-io completes: %d bytes\n", (int)iocount);}    
            break;

        case kPW:
            if (pPioregn == NULL)
                {
                rc = -ENOMEM;
                break;
                }

            memset(pPioregn, '.' /*0x00*/, MAD_SECTOR_SIZE);
	    memcpy(pPioregn, (char *)parm, strlen((char *)parm));
            //fprintf(stderr, "Write to PIO region completes: %d bytes\n%s\n", val, (char *)parm);
            fprintf(stderr, "Write to PIO region completes: %d bytes\n", val);
            //fprintf(stderr, "Write to PIO region completes: \n%s\n", (char *)pPioregn);
            break;

        case kPR:
            if (pPioregn == NULL)
                {
                rc = -ENOMEM;
                break;
                }

            memset(iobufr, 0x00, MAD_SECTOR_SIZE);
	    memcpy(iobufr, pPioregn, (size_t)val);
	    //fprintf(stderr, "Read from PIO region completes: %d bytes...\n%s\n", val, pPioregn);
	    fprintf(stderr, "Read from PIO region completes: %d bytes...\n%s\n", val, iobufr);
            break;

 	case kMAP:
	    rc = MapDeviceRegs(&pMapdDevRegs, fd);
	    break;

	case kGET:
	    rc = ioctl(fd, MADDEVOBJ_IOC_GET_DEVICE, &MadCtlParms);
	    if (rc == 0)
		{DisplayDevRegs(&MadCtlParms.MadRegs);}
	    break;

	case kMGT:
	    if ((pMapdDevRegs == NULL) || ((long int)pMapdDevRegs == -1))
	        rc = -ENOSYS; 
            else
	        DisplayDevRegs(pMapdDevRegs);
	    break;

        case kPRC:
            memset(iobufr, '.', MAD_CACHE_SIZE_BYTES);
	    rc = ioctl(fd, MADDEVOBJ_IOC_PULL_READ_CACHE, iobufr);
	    if (rc == 0)
                {fprintf(stderr, "Pull read cache completes: %d bytes...\n%s\n",
            MAD_CACHE_SIZE_BYTES, iobufr);}
	    break;

	case kPWC:
	    memset(iobufr, 0x00, MAD_CACHE_SIZE_BYTES);
	    memcpy((char *)iobufr, (char *)parm, strlen((char *)parm));
	    rc = ioctl(fd, MADDEVOBJ_IOC_PUSH_WRITE_CACHE, iobufr);
	    if (rc == 0)
                {fprintf(stderr, "Push write cache completes: %d bytes...\n", MAD_CACHE_SIZE_BYTES);}
	    break;

	case kARC:
	    rc = ioctl(fd, MADDEVOBJ_IOC_ALIGN_READ_CACHE, val);
	    break;

	case kAWC:
	    rc = ioctl(fd, MADDEVOBJ_IOC_ALIGN_WRITE_CACHE, val);
	    break;

	/*case kGEN: //get int-enable
	    rc = ioctl(fd, MADDEVOBJ_IOC_GET_ENABLE, 0, &regval);
	    break;

	case kGCT: //get control
	    rc = ioctl(fd, MADDEVOBJ_IOC_GET_CONTROL, 0, &regval);
	    break;*/

	case kSET: //set int-enable
	    switch (iparm)
		{
		case kIEN:
		    rc = ioctl(fd, MADDEVOBJ_IOC_SET_ENABLE, val);
		    break;

		case kCTL: //set control
		     rc = ioctl(fd, MADDEVOBJ_IOC_SET_CONTROL, val);
		     break;

		default:
		    assert(false);
		     rc = -EINVAL;
	        }
  	    break;

	default:
	    assert(false);
	    rc = -EINVAL;
	    }

    if (rc == -1)
        {rc = -errno;}

    if (rc != 0)
	fprintf(stderr, "Process_Cmd returning %d\n", rc);

    return rc;
}


int GetBufr(char** ppBufr, size_t IoSize)
{
static size_t CurBufrSize = 0;

    if (*ppBufr != NULL) //We have a buffer
	{
	if (CurBufrSize >= IoSize) //It's big enough
            {return 0;}
	else
	    {free(*ppBufr);}
	}

    *ppBufr = (char *)malloc(IoSize);
    if (*ppBufr == NULL)
	{
	fprintf(stderr, "madtest: malloc failure, errno=%d\n", errno);
    	return -errno;
	}

    CurBufrSize = IoSize;
    return 0;
}

void InitData(char* pBufr, size_t Len)
{
static char Data[] = "0123456789ABCDEF";
register int j;

    for (j = 0; j < (int)Len; j++)
        pBufr[j] = Data[j%16];

    return;
}


void DisplayData(char* pBufr, size_t Len)
{
register int j;

    for (j = 0; j < (int)64; j++)
        printf("%c,", pBufr[j]);
    printf("\n");

    for (j = 0; j < (int)Len; j++)
        printf("%2X,", pBufr[j]);
    printf("\n");

    return;
}

ssize_t Async_Io(int fd, u8* pBufr, size_t DataLen, size_t offset, u8 bWrite)
{
int rc = 0;
aiocb AioCb;
 
    memset(&AioCb, 0x00, sizeof(aiocb));
    AioCb.aio_nbytes = DataLen;
    AioCb.aio_fildes = fd;
    AioCb.aio_offset = offset;
    AioCb.aio_buf    = pBufr;

    rc = (bWrite) ? aio_write(&AioCb) : aio_read(&AioCb);
    if (rc < 0)
        {
        fprintf(stderr, "Async_io transfer failure (%d)\n", errno);
        return errno;
        }

    rc = aio_error(&AioCb);
    while(rc == EINPROGRESS)
        {
        sleep(0);
        rc = aio_error(&AioCb);
        }

    return aio_return(&AioCb);
}

ssize_t Queued_Io(int fd, u8* pBufr, size_t DataLen, u8 bWrite)
{
struct iocb  Iocb0;
struct iocb* Iocbs[] = {&Iocb0, NULL, NULL, NULL};
struct io_event IoEvents[4];
io_context_t AioCtx = 0;
int rc = 0;

    //rc = syscall(__NR_io_setup, 1, &AioCtx);
    rc = io_setup(1, &AioCtx);
    if (rc < 0)
        {
        fprintf(stderr, "io_setup() returned (%d)\n", errno);
        return (ssize_t)rc;
        }

    memset(&Iocb0, 0x00, sizeof(struct iocb));
    Iocb0.aio_fildes     = fd;
    Iocb0.u.c.nbytes     = DataLen;
    Iocb0.u.c.offset     = 0;
    Iocb0.u.c.buf        = pBufr;
    Iocb0.aio_lio_opcode = (bWrite) ? IO_CMD_PWRITE : IO_CMD_PREAD;
    //
    //rc = syscall(__NR_io_submit, AioCtx, 1, Iocbs);
    rc = io_submit(AioCtx, 1, Iocbs);
    if (rc < 1)
        {
        fprintf(stderr, "io_submit() returned (%d), errno=%d\n", rc, errno);
        return (ssize_t)rc;
        }

    //Iocb0.u.c.nbytes = (U32)-1;
    //rc = syscall(__NR_io_getevents, AioCtx, 1, 1, IoEvents, NULL); //No time limit
    rc = io_getevents(AioCtx, 1, 1, IoEvents, NULL);
    if (rc < 1)
        {
        fprintf(stderr, "io_getevents() returned rc=%d, errno=%d\n", rc, errno);
        return (ssize_t)errno;
        }

    //rc = syscall(__NR_io_destroy, AioCtx);
    rc = io_destroy(AioCtx);
    if (rc < 0)
        {fprintf(stderr, "io_destroy() returned (%d)\n", errno);}

    return Iocb0.u.c.nbytes;
}

