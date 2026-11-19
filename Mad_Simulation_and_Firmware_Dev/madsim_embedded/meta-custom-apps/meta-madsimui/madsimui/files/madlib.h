/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2015 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */ 
/* XYZ Company                                                                 */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadBus.sys, MadDevice.sys, MadSimUI.exe, MadTestApp.exe,    */ 
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : MadDefinition.h                                             */
/*                                                                             */
/*  DESCRIPTION  : Properties & Definitions for the Model-Abstract Device      */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

#ifdef _MAIN_
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
//
int MapDeviceSection(void **ppSection, int fd, size_t MapLen, ULONG DevOfset)
{
	static ULONG MapFlags = MAP_SHARED; //necessary
	int rc = 0;

	*ppSection = mmap(NULL, /*LINUX_PAGE_SIZE*/MapLen, (PROT_READ|PROT_WRITE), MapFlags, fd, DevOfset);
	if (*ppSection == NULL)
	   	rc = -EPERM;
	else
		{
		if ((int)(*ppSection) == -1)
			rc = errno;
		else
		    ; //fprintf(stderr, "MapDeviceSection: mmap returns umaddr=%p\n", *ppSection);
		}

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
    fprintf(stderr, "Devnum=x%X MesgID=0x%X Control=0x%X Status=0x%X IntEnable=0x%X IntID=0x%X\n",
    		(unsigned int)pmadregs->Devnum, (unsigned int)pmadregs->MesgID,
    		(unsigned int)pmadregs->Control, (unsigned int)pmadregs->Status,
		(unsigned int)pmadregs->IntEnable, (unsigned int)pmadregs->IntID);
    fprintf(stderr, "ReadDX=0x%X  WriteDX=0x%X  ReadCacheDx=0x%X  WriteCacheDx=0x%X  PwrState=0x%X\n",
    		(unsigned int)pmadregs->ByteIndxRd, (unsigned int)pmadregs->ByteIndxWr,
			(unsigned int)pmadregs->CacheIndxRd, (unsigned int)pmadregs->CacheIndxWr, (unsigned int)pmadregs->PowerState);

    return;
}
//
#endif  //_MAIN_

#ifdef _DEVICE_DRIVER_MAIN_
void maddev_program_bufrd_io(spinlock_t *splock, PMADREGS pmadregs,
		               ULONG ControlReg, ULONG IntEnableReg, void *IoAddr)
{
	ULONG CntlReg = ControlReg;
    ULONG64 PhysAddr = __pa(IoAddr);

	spin_lock(splock);
	//
	iowrite32_rep(&pmadregs->HostAddr, &PhysAddr, 2);
    #ifdef MAD_SIMULATION_MODE
	//Treat the phys addr in the hardware register as a virt addr
	//memcpy(&pmadregs->HostAddr, IoAddr, sizeof(void *));
	pmadregs->HostAddr = (u64)(u32)IoAddr;
    #endif

	iowrite32(0, &pmadregs->IntID);
	iowrite32(CntlReg, &pmadregs->Control);
	iowrite32(IntEnableReg, &pmadregs->IntEnable);
	//
	CntlReg |= MAD_CONTROL_BUFRD_GO_BIT;
	iowrite32(CntlReg, &pmadregs->Control);
	//
	spin_unlock(splock);

	return;
}

void MadResetIoRegisters(PMADREGS pmadregs, spinlock_t *splock)
{
	u32 Status;

	if (splock != NULL)
		spin_lock(splock);
	//
    iowrite32(0, &pmadregs->MesgID);
    iowrite32(MAD_CONTROL_RESET_STATE, &pmadregs->Control);
    Status = ioread32(&pmadregs->Status);
    Status &= ~MAD_STATUS_ERROR_MASK;
    iowrite32(Status, &pmadregs->Status);
    iowrite32(MAD_ALL_INTS_CLEARED, &pmadregs->IntID);
    iowrite32(MAD_ALL_INTS_ENABLED_MASK, &pmadregs->IntEnable);
	//
	if (splock != NULL)
		spin_unlock(splock);

	return;
}
#endif //_DEVICE_DRIVER_MAIN_

#endif  //_MADDEFS_
