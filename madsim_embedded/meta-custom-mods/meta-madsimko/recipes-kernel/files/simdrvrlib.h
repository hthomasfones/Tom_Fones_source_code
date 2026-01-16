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
/*  Exe files   : madbus.ko, maddevc.ko, maddevb.ko                            */ 
/*                                                                             */
/*  Module NAME : simdrvrlib.h                                                 */
/*                                                                             */
/*  DESCRIPTION : Function prototypes and definitions for the MAD bus driver   */
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
/* $Id: simdrvrlib.h, v 1.0 2021/01/01 00:00:00 htf $                          */
/*                                                                             */
/*******************************************************************************/
#ifndef _SIM_DRVR_LIB_
#define _SIM_DRVR_LIB_

struct pci_dev* madbus_setup_pci_device(U32 indx, U16 pci_devid);
int    madbus_hotplug(U32 indx, U16 pci_devid);
int    madbus_hotunplug(U32 indx);

void*  Get_KVA(phys_addr_t PhysAddr, struct page** ppPgStr);
void   SimInitPciCnfgSpace(char* PciCnfgSpace);
int    madbus_init_pci_config(PMADBUSOBJ pmadbusobj, bool bMSI);
void   madbus_init_pcidev(PMADBUSOBJ pmadbusobj, bool bMSI);

#ifdef _SIM_DRIVER_
//
#ifdef _DEVICE_DRIVER_MAIN_ 
//These defines belong in only one source module

extern int madbus_base_irq;
extern int madbus_nbr_slots;
extern PMADBUSOBJ madbus_objects;
extern MADBUS_ALLOC_ORDER geAllocOrder;
char *Version;
char madbus_id[];

//drivers/base/base.h
struct device_private 
{
	struct klist klist_children;
	struct klist_node knode_parent;
	struct klist_node knode_driver;
	struct klist_node knode_bus;
	struct klist_node knode_class;
	struct list_head deferred_probe;
	struct device_driver *async_driver;
	struct device *device;
	u8 dead:1;
};

//Define these functions in only one source module
//Respond to udev events.
//
static int madbus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	if (add_uevent_var(env, "MADBUS_VERSION=%s", Version))
		{return -ENOMEM;}

	return 0;
}

/*
 * Match LDD devices to drivers.  Just do a simple name test.
 */
static int madbus_match(struct device *dev, struct device_driver *driver)
{
	//return !strncmp(dev_name(dev), driver->name, strlen(driver->name));
    return 1;
}
//The bus type definition.
//
struct bus_type madbus_type =
{
	.name = "madbus",
	.match = madbus_match,
	.uevent = madbus_uevent,
};
//
struct attribute madbus_attr =
{
    .name = "madbus_attr",
    .mode = 1, 
};
//
struct bus_attribute madbus_attr_ver =
{
	.attr = {.name = "madbus_attr", .mode=1,},
	.show = NULL,
	.store = NULL,
};

//The driver definition
struct device_driver madbus_drvr =
{
    .name  = "madbus",
    .bus   = &madbus_type,
    .owner = THIS_MODULE,
};

// madbus_dev becomes the root of the bus object device tree
struct device madbus_root =
{
    .parent    = NULL,
	.init_name = madbus_id, 
    .driver    = NULL,
	.release   = madbus_root_release, 
};

//Crude driver interface.
/*
static ssize_t show_version(struct device_driver *driver, char *buf)
{
	struct mad_driver *ldriver = to_mad_driver(driver);

	sprintf(buf, "%s\n", ldriver->version);
	return strlen(buf);
}
*/
static struct driver_attribute DrvrAttr =
{
    //.mode = S_IRUGO,
    .show = NULL, //show_version,
    .store = NULL,
};

static struct pci_driver* PciDrvrs[MADBUS_NUMBER_SLOTS] = {NULL, NULL, NULL};
static U8     bDrvSysfs[MADBUS_NUMBER_SLOTS] = {0, 0, 0};
static U32    NumDrvrs = 0;

void sim_init_sysfs_device(struct device *pDevice, int devnum, 
                           const char* devname, const struct device_driver *pdriver,
                           void* drvrdata, void	(*release_fn)(struct device *));

void sim_init_sysfs_device(struct device *pDevice, int devnum, 
                           const char* devname, const struct device_driver *pdriver,
                           void* drvrdata, void	(*release_fn)(struct device *))
{
    PMADBUSOBJ pmadbusobj = &madbus_objects[devnum];

    device_initialize(pDevice); //Not with device_register(); only with deice_add()
    dev_set_drvdata(pDevice, drvrdata);
    pDevice->bus         = &madbus_type;
    pDevice->parent      = &pmadbusobj->sysfs_dev;
    pDevice->release     = release_fn;
    pDevice->id          = devnum;
    //pDevice->p       = NULL; //assigned by the kernel
    dev_set_name(pDevice, devname);
}
EXPORT_SYMBOL_GPL(sim_init_sysfs_device);

//This function implements a simulation of the kernel/pcicore request_irq function
int  sim_request_irq(unsigned int irq, void* isrfunxn,
                     uint32_t flags, const char* dev_name, void* dev_id);
int  sim_request_irq(unsigned int irq, void* isrfunxn,
                     uint32_t flags, const char* dev_name, void* dev_id)
{
    uint32_t devnum;
    PMADBUSOBJ pmadbusobj;
    int irqdx = 0;
    U8  bMSI;
    //int rc = 0;

    if (dev_id == NULL)
        {return -ENODEV;}

    devnum = *(uint32_t*)dev_id; //assumes device# is at offset zero
    pmadbusobj = &madbus_objects[devnum];

    irqdx = (int)(irq - (U32)(madbus_base_irq));
    if ((irqdx < 0) || (irqdx > (MAD_NUM_MSI_IRQS + 1)))
       {return -EINVAL;}

    bMSI = pmadbusobj->pcidev.msi_cap;
    if (!bMSI)
        if (irqdx > 0)
            {return -EINVAL;}

    pmadbusobj->isr_devobj = dev_id;
    pmadbusobj->isrfn[irqdx] = isrfunxn;
    pmadbusobj->irq[irqdx] = irq;

    return 0;       
}

//This function implements a simulation of the kernel/pcicore free_irq function
int sim_free_irq(unsigned int irq, void* dev_id);
int sim_free_irq(unsigned int irq, void* dev_id)
{
    U32 devnum;
    int irqdx = -1;

    if (dev_id == NULL)
        {return -ENODEV;}

    devnum = *(U32*)dev_id; //assumes device# is at offset zero
    irqdx = (int)(irq - (U32)(madbus_base_irq));
    if ((irqdx < 0) || (irqdx > (MAD_NUM_MSI_IRQS + 1)))
       {return -EINVAL;}

    return 0;       
}
EXPORT_SYMBOL_GPL(sim_request_irq);
EXPORT_SYMBOL_GPL(sim_free_irq);

static char DfltPciConfig[MAD_PCI_CFG_SPACE_SIZE];
//
//Populate a template pci config space with generic data
void   SimInitPciConfig(U8* pPciCnfgSpace);
void   SimInitPciConfig(U8* pPciCnfgSpace)
{
    U16 class_dev = (0x05 << 8) | 0x00; /* base=0x05 (Mem-Cntrlr), subclass=0x00 */

    ASSERT((int)(VIRT_ADDR_VALID(pPciCnfgSpace, MAD_PCI_CFG_SPACE_SIZE)));
    memset(pPciCnfgSpace, 0x00, MAD_PCI_CFG_SPACE_SIZE);

    put_unaligned_le16(MAD_PCI_VENDOR_ID,      &pPciCnfgSpace[PCI_VENDOR_ID]);
	put_unaligned_le16(MAD_PCI_BASE_DEVICE_ID, &pPciCnfgSpace[PCI_DEVICE_ID]);
    put_unaligned_le16(class_dev, &pPciCnfgSpace[PCI_CLASS_DEVICE]);

    /* BARs (dummy values for template) */
	put_unaligned_le32(0xFFFF0000, &pPciCnfgSpace[PCI_BASE_ADDRESS_0]);
	put_unaligned_le32(0xFFFF1000, &pPciCnfgSpace[PCI_BASE_ADDRESS_1]);
	put_unaligned_le32(0xFFFF2000, &pPciCnfgSpace[PCI_BASE_ADDRESS_2]);
	put_unaligned_le32(0xFFFF3000, &pPciCnfgSpace[PCI_BASE_ADDRESS_3]);
	put_unaligned_le32(0xFFFF4000, &pPciCnfgSpace[PCI_BASE_ADDRESS_4]);
	put_unaligned_le32(0xFFFF5000, &pPciCnfgSpace[PCI_BASE_ADDRESS_5]);

	/* Subsystem IDs */
	put_unaligned_le16(MAD_PCI_VENDOR_ID,      
                       &pPciCnfgSpace[PCI_SUBSYSTEM_VENDOR_ID]);
	put_unaligned_le16(MAD_PCI_BASE_DEVICE_ID, 
                       &pPciCnfgSpace[PCI_SUBSYSTEM_ID]);

	/* Interrupts */
	pPciCnfgSpace[PCI_INTERRUPT_LINE] = 0x0A;
	pPciCnfgSpace[PCI_INTERRUPT_PIN]  = 0x01;  /* INTA# */

	return;
}

//The simulator replacement for register_device
//Nothing to do except register the device with sysfs
int sim_register_device(struct device* pDevice, u32 devnum, const char* devname);
int sim_register_device(struct device* pDevice, u32 devnum, const char* devname)
{
    PMADBUSOBJ pmadbusobj = &madbus_objects[devnum];
    int rc = 0;

    PINFO("sim_register_device... dev#=%d pDevice=%px\n", (int)devnum, pDevice);

    if (!pmadbusobj->bRegstrd)
        {
        rc = -ENODEV; //Not implemented because no parent sysfs_dev
        PWARN("sim_register_device:device_register... dev#=%d rc=%d\n",
              (int)devnum, (int)rc);
        return rc;
        }

    //Register the device  with sysfs as a child of the bus device
    //rc = device_register(pDevice);
    rc = device_add(pDevice);
    if (rc != 0)
        {
        put_device(pDevice);
        PWARN("sim_register_device:device_add... dev#=%d rc=%d\n",
              (int)devnum, (int)rc);
        }

    return rc;
}

#if 0
static void sim_device_unregister(struct device* pDevice)
{
    PINFO("sim_device_unregister... pDevice=%px id=%u\n", pDevice, pDevice->id);

    //device_unregister(pDevice);
    device_del(pDevice);
    put_device(pDevice);
}
#endif
void sim_unregister_device(struct device* pDevice);
void sim_unregister_device(struct device* pDevice)
{
    PINFO("sim_unregister_device... pDevice=%px id=%u\n", pDevice, pDevice->id);

    //Because we did add_device()
    device_del(pDevice);
    put_device(pDevice);
    pDevice->bus = NULL;
}
EXPORT_SYMBOL_GPL(sim_register_device);
EXPORT_SYMBOL_GPL(sim_unregister_device);

//This function implements a simulation of the equivalent pci function
//We save this pci-driver struct in a set to work w/ multiple client device drivers
int pcisim_register_driver(struct pci_driver *pcidrvr);
int pcisim_register_driver(struct pci_driver *pcidrvr)
{
	int rc = 0;
	//
    ASSERT((int)(pcidrvr != NULL));

	pcidrvr->driver.bus = &madbus_type;
	rc = driver_register(&pcidrvr->driver);
    //rc = bus_add_driver(&pcidrvr->driver);
	if (rc)
	    {
	    PERR("pcisim_register_driver:driver_register returned (%d)\n", rc);
	    return rc;
	    }

    PciDrvrs[NumDrvrs] = pcidrvr;

    DrvrAttr.attr.name = pcidrvr->driver.name;
    //DrvrAttr.attr.owner = pcidrvr->driver.owner;
	//rc = driver_create_file(&pcidrvr->driver, &DrvrAttr);
	if (rc != 0)
        {PWARN("pcisim_register_driver:driver_create_file returned (%d)\n", rc);}

    //bDrvSysfs[NumDrvrs] = (rc == 0);
    NumDrvrs++;
    PINFO("pcisim_register_driver... client driver=%s\n", pcidrvr->driver.name);

    //driver_create_file failure should not be fatal
    return 0;
}

//This function implements a simulation of the equivalent pci function
//We remove one pci device driver from the current set
//
void pcisim_unregister_driver(struct pci_driver *pcidrvr);
void pcisim_unregister_driver(struct pci_driver *pcidrvr)
{
    register U32 j;

    ASSERT((int)(pcidrvr != NULL));
	PINFO("pcisim_unregister_driver... pcidrvr=%px\n", pcidrvr);

    for (j=0; j < NumDrvrs; j++)
        {
        if (pcidrvr == PciDrvrs[j])
            {
           	PINFO("pcisim_unregister_driver... client driver=%s\n",
                   pcidrvr->driver.name);

            //if (bDrvSysfs[j] != 0)
            //    {driver_remove_file(&pcidrvr->driver, &DrvrAttr);}

	        driver_unregister(&pcidrvr->driver);
            //bus_remove_driver(&pcidrvr->driver);
            PciDrvrs[j] = PciDrvrs[NumDrvrs-1];
            bDrvSysfs[j] = bDrvSysfs[NumDrvrs-1];
            PciDrvrs[NumDrvrs-1] = NULL;
            NumDrvrs--;
            break;
            }
        }

    return;
}
EXPORT_SYMBOL_GPL(pcisim_register_driver);
EXPORT_SYMBOL_GPL(pcisim_unregister_driver);

//These structures are useful to spoof the device_bound_to_driver state 
//in simulation mode
//static struct klist maddev_klist;
//static struct list_head empty_list = {&empty_list, &empty_list};

#if 0
static struct device_private maddev_priv_data1 = 
{
    .knode_driver   = {.n_klist = &maddev_klist,}, //avoid a null-pntr at unregister

    //set the list to empty by pointers to itself next,prev
    .deferred_probe = {&maddev_priv_data1.deferred_probe,
                       &maddev_priv_data1.deferred_probe,},
    .dead = 0,
};
static struct device_private maddev_priv_data2 = 
{
    .knode_driver   = {.n_klist = &maddev_klist,}, //avoid a null-pntr at unregister

    //set the list to empty by pointers to itself next,prev
    .deferred_probe = {&maddev_priv_data2.deferred_probe,
                       &maddev_priv_data2.deferred_probe,},
    .dead = 0,
};
static struct device_private maddev_priv_data3 = 
{
    .knode_driver   = {.n_klist = &maddev_klist,}, //avoid a null-pntr at unregister

    //set the list to empty by pointers to itself next,prev
    .deferred_probe = {&maddev_priv_data3.deferred_probe,
                       &maddev_priv_data3.deferred_probe,},
    .dead = 0,
};
struct device_private* maddev_priv_data[4] = 
       {NULL, &maddev_priv_data1, &maddev_priv_data2, &maddev_priv_data3};
#endif //0

// This function implements a simulation of the equivalent pci function
// It returns an initialized pci_dev from the next available bus slot
//
struct pci_dev* pcisim_get_device(unsigned int vendor, unsigned int pci_devid, 
                                  struct pci_dev *start);
struct pci_dev* pcisim_get_device(unsigned int vendor, unsigned int pci_devid, 
                                  struct pci_dev *start)
{
    PMADBUSOBJ pmadbusobj; 
    int        indx = 0;

    if ((vendor != MAD_PCI_VENDOR_ID) ||
        (pci_devid < MAD_PCI_BASE_DEVICE_ID) || (pci_devid > MAD_PCI_MAX_DEVICE_ID))
        {
       	PERR("pcisim_get_device vendor:device_id mismatch... rc=-EINVAL\n");
        return ERR_PTR(-EINVAL);
        }

    if (start != NULL)
        {  //First we must find the passed in pci_dev pntr
        for (indx=1; indx <= madbus_nbr_slots; indx++)
            {
            pmadbusobj = &madbus_objects[indx];
            if  (&pmadbusobj->pcidev == start)
                {break;}
            }

        if (indx >= madbus_nbr_slots) //There are no more free bus slots 
            {return ERR_PTR(-EBADSLT);}
        }

    //We are returning a pcidev - let's set it up 1st
    return madbus_setup_pci_device((indx+1), pci_devid);
}
EXPORT_SYMBOL_GPL(pcisim_get_device);

//This function implements a simulation of the equivalent pci function
int pcisim_enable_device(struct pci_dev* pPcidev);
int pcisim_enable_device(struct pci_dev* pPcidev)
{
   	struct madbus_object *mbobj = NULL;

    if (pPcidev == NULL)
        {return -ENODEV;}

    mbobj = container_of(pPcidev, struct madbus_object, pcidev);

    PINFO("pcisim_enable_device... dev#=%d mbobj=%px pcidev=%px base_irq=%d isrfn_0=%px\n",
		  (int)mbobj->devnum, mbobj, &mbobj->pcidev, pPcidev->irq, mbobj->isrfn[0]);

    return 0;
}
EXPORT_SYMBOL_GPL(pcisim_enable_device);

//This function implements a simulation of the equivalent pci function
int pcisim_disable_device(struct pci_dev* pPcidev);
int pcisim_disable_device(struct pci_dev* pPcidev)
{
   	struct madbus_object *mbobj;

    if (pPcidev == NULL)
        {return -ENODEV;}

    mbobj = container_of(pPcidev, struct madbus_object, pcidev);
    PINFO("pcisim_disable_device... dev#=%d mbobj=%px pcidev=%px\n",
		  (int)mbobj->devnum, mbobj, &mbobj->pcidev);

    return 0;
}
EXPORT_SYMBOL_GPL(pcisim_disable_device);

//This function implements a simulation of the equivalent pci function
int pcisim_enable_msi_block(struct pci_dev* pPcidev, int num);
int pcisim_enable_msi_block(struct pci_dev* pPcidev, int num)
{
   struct madbus_object *mbobj;

   if (pPcidev == NULL)
        {return -ENODEV;}

   // All MSIs plus one legacy-int - this is a little sloppy
   if (num > (MAD_NUM_MSI_IRQS+1))
       {return -EINVAL;}

    mbobj = container_of(pPcidev, struct madbus_object, pcidev);
    PINFO("pcisim_enable_msi_block... dev#=%d mbobj=%px pcidev=%px rc=0\n",
		  (int)mbobj->devnum, mbobj, &mbobj->pcidev);

    return 0;
}
EXPORT_SYMBOL_GPL(pcisim_enable_msi_block);

//This function implements a simulation of the equivalent pci function
void pcisim_disable_msi(struct pci_dev* pPcidev);
void pcisim_disable_msi(struct pci_dev* pPcidev)
{
   	struct madbus_object *mbobj;

    if (pPcidev == NULL)
        {return;}

    mbobj = container_of(pPcidev, struct madbus_object, pcidev);
    PINFO("pcisim_disable_msi... dev#=%d mbobj=%px pcidev=%px\n",
		  (int)mbobj->devnum, mbobj, &mbobj->pcidev);

    return;
}
EXPORT_SYMBOL_GPL(pcisim_disable_msi);

//This function implements a simulation of the equivalent pci function
int pcisim_read_config_byte(const struct pci_dev* pPcidev, int where, u8 *val);
int pcisim_read_config_byte(const struct pci_dev* pPcidev, int where, u8 *val)
{
    int rc = 0;
	struct madbus_object *mbobj = 
                         container_of(pPcidev, struct madbus_object, pcidev);
	U8*        pPciConfig;

	if (mbobj == NULL)
		{rc = -ENODEV;}

    if ((where > MAD_PCI_CFG_SPACE_SIZE) || (where < 0))
	    {rc = -EINVAL;}

    if (rc == 0)
        {    
	    pPciConfig = mbobj->PciConfig;
	    *val = pPciConfig[where];
        }

	PINFO("pcisim_read_config_byte... dev#=%d mbobj=%px pcidev=%px val=%d rc=%d\n",
		   (int)mbobj->devnum, mbobj, &mbobj->pcidev, *val, rc);

    return rc;
}

//This function implements a simulation of the equivalent pci function
int pcisim_read_config_word(const struct pci_dev* pPcidev, int where, u16 *val);
int pcisim_read_config_word(const struct pci_dev* pPcidev, int where, u16 *val)
{
	struct madbus_object *mbobj = container_of(pPcidev, struct madbus_object, pcidev);
	U8*        pPciConfig;
    u16*       pPciCnfg16;

	if (mbobj == NULL)
		{return -ENODEV;}

	if ((where > MAD_PCI_CFG_SPACE_SIZE) || (where < 0))
	    {return -EINVAL;}

	PINFO("pcisim_read_config_word... dev#=%d mbobj=%px pcidev=%px\n",
		   (int)mbobj->devnum, mbobj, &mbobj->pcidev);

	pPciConfig = mbobj->PciConfig;
    pPciCnfg16 = (U16*)&(pPciConfig[where]);
	*val = *pPciCnfg16;

    return 0;
}

//This function implements a simulation of the equivalent pci function
int pcisim_read_config_dword(const struct pci_dev *pPcidev, 
                             int offset, uint32_t* val);
int pcisim_read_config_dword(const struct pci_dev *pPcidev, 
                             int offset, uint32_t* val)
{
    int rc = 0;
	struct madbus_object *mbobj = 
    container_of(pPcidev, struct madbus_object, pcidev);
	U8*        pPciConfig;
    uint32_t*  pPciCnfg32;

	if (mbobj == NULL)
		{rc = -ENODEV;}

	if ((offset > MAD_PCI_CFG_SPACE_SIZE) || (offset < 0))
	    {rc = -EINVAL;}

    if (rc == 0)
        {       
	    pPciConfig = mbobj->PciConfig;
        pPciCnfg32 = (uint32_t *)&(pPciConfig[offset]);
	    *val = *pPciCnfg32;
        }

	PINFO("pcisim_read_config_dword... dev#=%d mbobj=%px pcidev=%px offset=%d val=%d rc=%d\n",
		  (int)mbobj->devnum, mbobj, &mbobj->pcidev, offset, *val, rc);

    return rc;
}
EXPORT_SYMBOL_GPL(pcisim_read_config_byte);
EXPORT_SYMBOL_GPL(pcisim_read_config_word);
EXPORT_SYMBOL_GPL(pcisim_read_config_dword);

//This function implements a simulation of the equivalent pci function
int  pcisim_request_region(struct pci_dev *pdev, int bar, char* resname);
int  pcisim_request_region(struct pci_dev *pdev, int bar, char* resname)
{
    int rc = 0;
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {rc = -ENODEV;}
    
    if (bar != 0)
        {rc = -EINVAL;}

    pdev->irq = madbus_base_irq;
    PINFO("pcisim_request_region... dev#=%d pdev=%px bar=%d irq=%d rc=%d\n",
		  (int)pmadbusobj->devnum, pdev, bar, pdev->irq, rc);

    return rc;
}

//This function implements a simulation of the equivalent pci function
void  pcisim_release_region(const struct pci_dev *pdev, int bar);
void  pcisim_release_region(const struct pci_dev *pdev, int bar)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    int rc = 0;
    if (pmadbusobj->pci_devid == 0)
        {rc = -ENODEV;}

    if (bar > 0)
        {rc = -EINVAL;}

    PINFO("pcisim_release_region... dev#=%d pdev=%px bar=%d rc=%d\n",
		  (int)pmadbusobj->devnum, pdev, bar, rc);

    return;
}
EXPORT_SYMBOL_GPL(pcisim_request_region);  
EXPORT_SYMBOL_GPL(pcisim_release_region);  

//This function implements a simulation of the equivalent pci function
phys_addr_t pcisim_resource_start(const struct pci_dev *pdev, int bar);
phys_addr_t pcisim_resource_start(const struct pci_dev *pdev, int bar)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    phys_addr_t BarAddr = (U64)0;

    if (pmadbusobj->pci_devid == 0)
        {BarAddr = (0LL)-1;}
    else
        {
        switch (bar)
            {
            case 0:
                BarAddr = pmadbusobj->MadDevPA;  
                break;

             case 2:
                 BarAddr = (pmadbusobj->MadDevPA + MADREGS_BASE_SIZE);  
                 break;

             case 4:
                 BarAddr = (pmadbusobj->MadDevPA + MAD_MAPD_READ_OFFSET);    
                 break;

            default:
                BarAddr = (U64)-EINVAL;
            }
        }

    PINFO("pcisim_resource_start... devnum=%d bar=%d BarAddr=%llx\n", 
          (int)pmadbusobj->devnum, bar, BarAddr);

   return BarAddr;
}
EXPORT_SYMBOL_GPL(pcisim_resource_start);  

//This function implements a simulation of the equivalent pci function
U64 pcisim_resource_end(const struct pci_dev *pdev, int bar);
U64 pcisim_resource_end(const struct pci_dev *pdev, int bar)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    U64 RegnEnd = 0;

    if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

    switch (bar)
        {
        case 0:
            RegnEnd = (pmadbusobj->MadDevPA + MADREGS_BASE_SIZE);  
            break;

         case 2:
             RegnEnd = (pmadbusobj->MadDevPA + MAD_REGISTER_BLOCK_SIZE);  
             break;

         case 4:
             RegnEnd = (pmadbusobj->MadDevPA + MAD_DEVICE_DATA_OFFSET);    
             break;

        default:
            RegnEnd = (U64)-EINVAL;
        }

    PINFO("pcisim_resource_end... devnum=%d bar=%d RegnEnd=%llx\n", 
          (int)pmadbusobj->devnum, bar, RegnEnd);

   return RegnEnd;
}
EXPORT_SYMBOL_GPL(pcisim_resource_end);  

//This function implements a simulation of the equivalent pci function
U32 pcisim_resource_len(const struct pci_dev *pdev, int bar);
U32 pcisim_resource_len(const struct pci_dev *pdev, int bar)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

    if (bar != 0)
       {return -EINVAL;}

    return MADREGS_BASE_SIZE;
}
EXPORT_SYMBOL_GPL(pcisim_resource_len);  

//This function implements a simulation of the equivalent pci function
U32 pcisim_resource_flags(const struct pci_dev *pdev, int bar);
U32 pcisim_resource_flags(const struct pci_dev *pdev, int bar)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

   if (bar > 0)
       {return -EINVAL;}

    return IORESOURCE_MEM;
}
EXPORT_SYMBOL_GPL(pcisim_resource_flags);  

int pcisim_alloc_irq_vectors(struct pci_dev *pdev, unsigned int min_vecs, 
		                     unsigned int max_vecs, unsigned int flags);
int pcisim_alloc_irq_vectors(struct pci_dev *pdev, unsigned int min_vecs, 
		                     unsigned int max_vecs, unsigned int flags)
{
    int rc; 
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

    PINFO("pcisim_alloc_irq_vectors... dev#=%d min_vecs=%u max_vecs=%u flags=%u rc=%d\n",
          (int)pmadbusobj->devnum, min_vecs, max_vecs, flags, rc);  
          
    return rc;
}

void pcisim_free_irq_vectors(struct pci_dev *pdev);
void pcisim_free_irq_vectors(struct pci_dev *pdev)
{
    int rc = 0; 

    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {rc =-ENODEV;}

    PINFO("pcisim_free_irq_vectors... dev#=%d rc=%d\n",
          (int)pmadbusobj->devnum, rc);      
}   
EXPORT_SYMBOL_GPL(pcisim_alloc_irq_vectors);  
EXPORT_SYMBOL_GPL(pcisim_free_irq_vectors);  

void __iomem* pcisim_iomap(struct pci_dev *pdev, int bar, unsigned long size);
void __iomem* pcisim_iomap(struct pci_dev *pdev, int bar, unsigned long size)
{
    int rc = 0;
    __iomem U8 *pdevaddr = NULL;
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {rc = -ENODEV;}

    if (bar != 0)
        {rc = -EINVAL;}
    
    if (rc == 0)
        {
        if (geAllocOrder == eDtbPrivMem) //Reserved nomap memory in device-tree
            {pdevaddr = devm_ioremap(&madbus_root, pmadbusobj->MadDevPA, size);}
        else
            {pdevaddr = (void __iomem *)phys_to_virt(pmadbusobj->MadDevPA);}
        }

    PINFO("pcisim_iomap... dev#=%d hDMA=%llx allocVA=%px devPA=%llX pdevaddr=%px size=%lu rc=%d\n",
          (int)pmadbusobj->devnum, pmadbusobj->hDMA, pmadbusobj->pmadregs, 
          pmadbusobj->MadDevPA, pdevaddr, size, rc);        

    return pdevaddr;
}
EXPORT_SYMBOL_GPL(pcisim_iomap);

void pcisim_iounmap(struct pci_dev *pdev, void __iomem *pdevaddr);
void pcisim_iounmap(struct pci_dev *pdev, void __iomem *pdevaddr)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    int rc = 0;

    if (pmadbusobj->pci_devid == 0)
        {rc = -EINVAL;}

    PINFO("pcisim_iounmap.. . dev#=%d devPA=%llX pdevaddr=%px rc=%d\n",
          (int)pmadbusobj->devnum, pmadbusobj->MadDevPA, pdevaddr, rc);        
}
EXPORT_SYMBOL_GPL(pcisim_iounmap);  

//This function provides a pointer to a parm exchange area so that
//the client driver can operate in simulation mode
PMAD_SIMULATOR_PARMS madbus_exchange_parms(int num);
PMAD_SIMULATOR_PARMS madbus_exchange_parms(int num)
{
	PMADBUSOBJ pmadbusobj = &madbus_objects[num];
    __iomem MADREGS *pmadregs = pmadbusobj->pmadregs;
    if (pmadregs == NULL) //We didn't get ram
        {return NULL;}

    //The device knows it's own slot-id (dev#)   
    iowrite32((uint32_t)pmadbusobj->devnum, &pmadregs->Devnum);
 
    //The simulator's device object contains the pcidev struct
    pmadbusobj->SimParms.pPcidev = &pmadbusobj->pcidev;
    pmadbusobj->SimParms.pcomplete_simulated_io = madsim_complete_simulated_io;
    pmadbusobj->SimParms.pmadbusobj = pmadbusobj;

    PINFO("madbus_exchange_parms... dev#=%d pmbobj=%px pPciDev=%px\n",
          (int)pmadbusobj->devnum, pmadbusobj, pmadbusobj->SimParms.pPcidev); 
          //pmadbusobj->SimParms.pcomplete_simulated_io); /* */

	return &pmadbusobj->SimParms;
}
EXPORT_SYMBOL_GPL(madbus_exchange_parms);

//This function simulates a PCI device hotplug
int madbus_hotplug(U32 indx, U16 pci_devid)
{
    register U32 j;
    //
    PMADBUSOBJ pmbobj_hpl = &madbus_objects[indx];
    struct pci_driver* pPciDrvr;
    struct pci_dev*    pPcidev = NULL;
    int                rc = -EUNATCH; //No 'protocol' driver attached until we find one
    
    PINFO("madbus_hotplug... indx=%d devnum=%d pci_devid=x%X\n", 
          (int)indx, (int)pmbobj_hpl->devnum, pci_devid);

    if ((indx < 1) || (indx > madbus_nbr_slots))
        {return -EBADSLT;} //Slot# out of bounds 

    if (pmbobj_hpl->pci_devid != 0)
       {return -EADDRINUSE;} //This bus slot is in use;

    pPcidev = madbus_setup_pci_device(indx, pci_devid);
    if ((pPcidev == NULL) || (IS_ERR(pPcidev)))
        {return -EFAULT;}

    ASSERT((int)(pPcidev == &pmbobj_hpl->pcidev));

    //Find the correct device driver from the set of known client drivers
    //Calling every drivers' probe function until a match or error/no match
    for (j = 0; j < NumDrvrs; j++)
        {
        pPciDrvr = PciDrvrs[j];
        if (pPciDrvr == NULL)
            {continue;}

        ASSERT((int)(pPciDrvr->probe != NULL));
        if (pPciDrvr->probe == NULL)
            {continue;}

        rc = pPciDrvr->probe(&pmbobj_hpl->pcidev, pPciDrvr->id_table);
        if (rc != MAD_NO_VENDOR_DEVID_MATCH) // Found!
            {break;}
        }

    //if any probe was invoked the hotplug result is 
    // success or some internal error or no driver claimed the device 
    if (rc == MAD_NO_VENDOR_DEVID_MATCH) 
        {rc = -EUNATCH;} /* Protocol driver not attached */

    if (rc != 0)
        //Mark this slot as free
        {
        PWARN("madbus_hotplug... dev#=%d rc=%d\n", (int)indx, rc);
        pmbobj_hpl->pci_devid = 0;
        }
    else
        {
        ASSERT((int)(pmbobj_hpl->pcidev.driver != NULL));
        pmbobj_hpl->bHP = 1;
        }

    return rc;
}

//This function simulates a PCI device hot unplug
int madbus_hotunplug(U32 indx)
{
    PMADBUSOBJ pmbobj_hpl = &madbus_objects[indx];
    //
    struct pci_driver *pPciDrvr;
    int rc = 0; 

    PINFO("madbus_hotunplug... pmobj=%px dev#=%d\n", pmbobj_hpl, (int)indx);

    if ((indx < 1) || (indx > madbus_nbr_slots))
        {return -EBADSLT;} //Slot# out of bounds 

    if (pmbobj_hpl->pci_devid == 0)
       {return -ENODEV;} //This bus slot has no device;

    if (!pmbobj_hpl->bHP)
        {return -EOPNOTSUPP;} //Operation not supported - this device wasn't hot-plugged

    pPciDrvr = pmbobj_hpl->pcidev.driver; 
    if (pPciDrvr == NULL)
        {return -EUNATCH;} //No 'protocol' driver attached 

    if (pPciDrvr->remove == NULL)
       {return -EFAULT;} //Internal fault - no remove function is set up

    /*rc =*/ pPciDrvr->remove(&pmbobj_hpl->pcidev);
    rc = 0;

    pmbobj_hpl->pci_devid = 0;

    //The final result of the unplug is ok if any remove was invoked
    return 0;
}

// This function sets up and initializes one pci device
// whether found on the bus or hotplugged
struct pci_dev* madbus_setup_pci_device(U32 indx, U16 pci_devid)
{
    int rc;
    PMADBUSOBJ pmadbusobj = &madbus_objects[indx];
    pmadbusobj->devnum = indx;
    pmadbusobj->pci_devid = pci_devid;
    bool      bMSI = (pci_devid % 2) == 0;

    PINFO("madbus_setup_pci_device... dev#=%d mbobj=%px pcidevid=x%X pcidev=%px\n",
		  (int)indx, pmadbusobj, pci_devid, &(pmadbusobj->pcidev));

    rc = madbus_init_pci_config(pmadbusobj, bMSI);
    if (rc != 0)
        {
   	    PERR("madbus_setup_pci_device... dev#=%d rc=%d\n", (int)pmadbusobj->devnum, rc);
        pmadbusobj->pci_devid = 0;
        return ERR_PTR(rc);
        }
 
    madbus_init_pcidev(pmadbusobj, bMSI);

    return &(pmadbusobj->pcidev);
}

//This function returns a kernel virtual address whether or not the
//physical address is currently mapped into kernel virt addr space
void* Get_KVA(phys_addr_t PhysAddr, struct page** ppPgStr)
{
    phys_addr_t pfn = ((PhysAddr >> PAGE_SHIFT) & PFN_AND_MASK); // sign propogates Hi -> Lo  *!?!*
    struct page* pPgStr = pfn_to_page((int)pfn);
    void*  pvoid = page_address(pPgStr);
    bool   bKMAP = false;

    *ppPgStr = NULL;

    if (pvoid == NULL)
        {
        pvoid = kmap(pPgStr);
        bKMAP = true;
        *ppPgStr = pPgStr;
        }

    PINFO("Get_KVA... PA=x%lX pfn=x%llX pPgStr=%px kva=%px remap=%d\n",
           (unsigned long)PhysAddr, (unsigned long long)pfn,
           (void *)pPgStr, (void *)pvoid, (int)bKMAP);
    ASSERT((int)(pvoid != NULL));
    BUG_ON(pvoid == NULL);

    return pvoid;
}

//This function creates a custom PCI config space for a device
//starting from an initialized template
int madbus_init_pci_config(PMADBUSOBJ pmadbusobj, bool bMSI)
{
    static U32 AllFFs = 0xFFFFFFFF;
    //U8         bMSI = 0;
               //((pmadbusobj->pci_devid % 2) == 0); //Even# dev_ids are MSI-capable
	U8*        pPciCnfgParm = (U8*)(pmadbusobj->PciConfig);
    U16 msgctl = 0;

    u64 bar0 = (u64)pmadbusobj->MadDevPA;
    u64 bar2 = (u64)pmadbusobj->MadDevPA + MADREGS_BASE_SIZE;
    u64 bar4 = (u64)pmadbusobj->MadDevPA + MAD_MAPD_READ_OFFSET;
    
    U64 SimParmsAddr = 0;

	if (pmadbusobj->pmadregs == NULL)
		{return -ENODEV;}
	
	memcpy(pPciCnfgParm, DfltPciConfig, MAD_PCI_CFG_SPACE_SIZE);
   	put_unaligned_le16((u16)pmadbusobj->pci_devid, &pPciCnfgParm[PCI_DEVICE_ID]);
    put_unaligned_le16((u16)pmadbusobj->pci_devid, 
                       &pPciCnfgParm[PCI_SUBSYSTEM_ID]);

    /* write the BARs as LE32 pairs */
    put_unaligned_le32((u32)(bar0 & AllFFs), 
                       &pPciCnfgParm[PCI_BASE_ADDRESS_0]);
    put_unaligned_le32((u32)(bar0 >> 32), 
                       &pPciCnfgParm[PCI_BASE_ADDRESS_0 + 4]);

    put_unaligned_le32((u32)(bar2 & AllFFs), 
                       &pPciCnfgParm[PCI_BASE_ADDRESS_2]);
    put_unaligned_le32((u32)(bar2 >> 32), &pPciCnfgParm[PCI_BASE_ADDRESS_2 + 4]);

    put_unaligned_le32((u32)(bar4 & AllFFs),
                       &pPciCnfgParm[PCI_BASE_ADDRESS_4]);
    put_unaligned_le32((u32)(bar4 >> 32), &pPciCnfgParm[PCI_BASE_ADDRESS_4 + 4]);

    pPciCnfgParm[PCI_INTERRUPT_LINE] = 
                 (U8)(madbus_base_irq + pmadbusobj->devnum);
    pPciCnfgParm[PCI_INTERRUPT_PIN] = pmadbusobj->devnum;

    //Initialize MSI    
    pPciCnfgParm[PCI_CAPABILITY_LIST] = 
                 (bMSI) ? PCI_CONFIG_MSI_CAP_OFFSET : 0x00;
    if (bMSI) 
        {
        pPciCnfgParm[PCI_CONFIG_MSI_CAP_OFFSET] = PCI_CAP_ID_MSI;
        pPciCnfgParm[PCI_CONFIG_MSI_CAP_OFFSET+1] = 0x00;
        put_unaligned_le16(msgctl, &pPciCnfgParm[PCI_CONFIG_MSI_CAP_OFFSET+2]);

        /* message address/data fields start zero; OS will write them */
        put_unaligned_le32(0, &pPciCnfgParm[PCI_CONFIG_MSI_CAP_OFFSET+4]); /* addr low */
        put_unaligned_le32(0, &pPciCnfgParm[PCI_CONFIG_MSI_CAP_OFFSET+8]); /* addr high if 64-bit */
        put_unaligned_le16(0, &pPciCnfgParm[PCI_CONFIG_MSI_CAP_OFFSET+12]); /* data */
        }

    //Set the pointer to the simulation parms at a known location
    SimParmsAddr = (U64)&pmadbusobj->SimParms;
    put_unaligned_le32((U32)(SimParmsAddr & AllFFs), 
                       &pPciCnfgParm[MAD_PCI_VENDOR_OFFSET]);
    put_unaligned_le32((u32)(SimParmsAddr >> 32), 
                       &pPciCnfgParm[MAD_PCI_VENDOR_OFFSET + 4]); 

    PINFO("madbus_init_pci_config... dev#=%d bMSI=%d PciCnfgBase0_loc,val=%px,x%llX\n",
          (int)pmadbusobj->devnum, bMSI, 
          (void *)&pPciCnfgParm[PCI_BASE_ADDRESS_0], bar0);

    return 0;
}

//This function creates a pci dev struct to provide to the pci device driver
void madbus_init_pcidev(PMADBUSOBJ pmadbusobj, bool bMSI)
{
    struct pci_dev* pPcidev; 
	U8*        pPciCnfgParm; 

    ASSERT((int)(pmadbusobj != NULL));
    pPcidev = &pmadbusobj->pcidev;
    memset(pPcidev, 0x00, sizeof(struct pci_dev));

    PINFO("madbus_init_pcidev... dev#=%d mbobj=%px\n",
          (int)pmadbusobj->devnum, pmadbusobj);

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_VENDOR_ID]);
    pPcidev->vendor = *(U16*)pPciCnfgParm;

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_DEVICE_ID]);
    pPcidev->device = *(U16*)pPciCnfgParm;

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_SUBSYSTEM_VENDOR_ID]);
    pPcidev->subsystem_vendor = *(U16*)pPciCnfgParm;

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_SUBSYSTEM_ID]);
    pPcidev->subsystem_device = *(U16*)pPciCnfgParm;

    pPcidev->cfg_size = MAD_PCI_CFG_SPACE_SIZE;

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_INTERRUPT_LINE]);
    //pPcidev->irq = pmadbusobj->base_irq; //(int)*pPciCnfgParm;

    pPcidev->msi_cap = pmadbusobj->PciConfig[PCI_CAPABILITY_LIST + PCI_CAP_ID_MSI];

    //We don't fully populate the pci slot struct 
    pPcidev->slot = &pmadbusobj->pciSlot;
    pPcidev->slot->number = (u8)pmadbusobj->devnum;
    pPcidev->dev.id = pmadbusobj->devnum;
    pPcidev->driver = NULL;//Should be assigned by the claiming device driver 

    return;
}
#endif //_DEVICE_DRIVER_MAIN_
#endif //_SIM_DRIVER_
//
#endif  //_SIM_DRVR_LIB_
