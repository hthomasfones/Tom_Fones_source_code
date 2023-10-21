
#ifndef __MADREGS
#define __MADREGS
typedef struct _MADREGS
    {
	unsigned long int MesgID;
	unsigned long int Control;
	unsigned long int Status;
	unsigned long int IntEnable;
	unsigned long int IntID;
    } MADREGS, *PMADREGS;

//* MAD REGISTER MASK BITS
//*
//Control
//
#define CONTROL_CACHE_XFER_BIT       0x00000001  //CX: Buffered I/O through the R/W cache 
#define CONTROL_IOSIZE_BYTES_BIT     0x00000002  //CB: Count is in bytes not sectors
#define CONTROL_BUFRD_GO_BIT         0x00000008  //BG: Buffered Go bit
//
#define CONTROL_CHAINED_DMA_BIT      0x00000040  //CD: DMA is chained
#define CONTROL_DMA_GO_BIT           0x00000080  //DG: DMA Go bit
//
#define CONTROL_IO_OFFSET_MASK       0x00000F00  //offset ... Control i-o offset
#define CONTROL_MAX_IO_SIZE_MASK     0x0000F000  //I/O size ..... Control i-o size 

//Status
//
#define STATUS_NO_ERROR_MASK         0x00000000  //    No indicated error
#define STATUS_GENERAL_ERR_BIT       0x00000001  //GE: general error
#define STATUS_OVER_UNDER_ERR_BIT    0x00000002  //OU: overflow/underflow
#define STATUS_DEVICE_BUSY_BIT       0x00000004  //DB: device busy 
#define STATUS_DEVICE_FAILURE_BIT    0x00000008  //DF: device failure
#define STATUS_INVALID_IO_BIT        0x00000100  //    not visible in the UI
#define STATUS_RESOURCE_ERROR_BIT    0x00000200  //    not visible in the UI
#define STATUS_TIMEOUT_ERROR_BIT     0x00010000  //    not visible in the UI
//
#define STATUS_READ_COUNT_MASK       0x000000F0  //    Read Count of completed input ...
#define STATUS_WRITE_COUNT_MASK      0x0000F000  //    Write Count of completed output ...

//Int-Enable; IntID
//
#define INT_BUFRD_INPUT_BIT          0x00000001  //Bu: Enable-indicate buffered input
#define INT_DMA_INPUT_BIT            0x00000002  //Dm: Enable-indicate DMA input
#define INT_ALIGN_INPUT_BIT          0x00000008  //CA: Enable-indicate read cache Alignment
//
#define INT_BUFRD_OUTPUT_BIT         0x00000100  //Bu: Enable-indicate buffered output
#define INT_DMA_OUTPUT_BIT           0x00000200  //Dm: Enable-indicate DMA ioutput
#define INT_ALIGN_OUTPUT_BIT         0x00000800  //CA: Enable-indicate write cache Alignment
//
#define INT_STATUS_ALERT_BIT         0x00008000  //SA: Enable-indicate Status Alert

#ifndef _ATOH_
int ATOH(char* hexstr)
{
register int j;
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

		indx = (int)pc - (int)HexDigits;
		val = (val << 4);
		val += indx;
	}

	return val;
}
#define _ATOH_
#endif
#endif
