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

static struct pci_dev* madbus_setup_pci_device(U32 indx, U16 pci_devid);
static int    madbus_hotplug(U32 indx, U16 pci_devid);
static int    madbus_hotunplug(U32 indx);

static void* Get_KVA(phys_addr_t PhysAddr, struct page** ppPgStr);
static void   SimInitPciCnfgSpace(char* PciCnfgSpace);
static int    madbus_set_pci_config(PMADBUSOBJ pmadbusobj);
static void   madbus_init_pcidev(PMADBUSOBJ pmadbusobj);

#ifdef _SIM_DRIVER_
//
#ifdef _DEVICE_DRIVER_MAIN_ 

int madbus_base_irq;
int madbus_nbr_slots;
PMADBUSOBJ madbus_objects;
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
//
//Respond to udev events.
//
static int mad_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	if (add_uevent_var(env, "MADBUS_VERSION=%s", Version))
		return -ENOMEM;

	return 0;
}

/*
 * Match LDD devices to drivers.  Just do a simple name test.
 */
static int mad_match(struct device *dev, struct device_driver *driver)
{
	return !strncmp(dev_name(dev), driver->name, strlen(driver->name));
}
//The bus type definition.
//
static struct bus_type madbus_type =
{
	.name = "madbus",
	.match = mad_match,
	.uevent = mad_uevent,
};
//
static struct attribute madbus_attr =
{
    .name = "madbus_attr",
    .mode = 1, 
};
//
static struct bus_attribute madbus_attr_ver =
{
	.attr = {.name = "madbus_attr", .mode=1,},
	.show = NULL,
	.store = NULL,
};

//The driver definition
static struct device_driver madbus_drvr =
{
    .name  = "madbus",
    .bus   = &madbus_type,
    .owner = THIS_MODULE,
};

//The bus device definition 
//
static void madbus_release(struct device *pdev)
{
	printk(KERN_DEBUG "madbus_release pdev=%px\n", pdev);
    ASSERT((int)(pdev != NULL));
}
//	
struct device madbus_dev =
{
    .parent    = NULL,
	.init_name = madbus_id, 
    .driver    = &madbus_drvr,
	.release   = madbus_release,
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
//
static struct driver_attribute DrvrAttr =
{
    //.mode = S_IRUGO,
    .show = NULL, //show_version,
    .store = NULL,
};

static struct pci_driver* PciDrvrs[MADBUS_NUMBER_SLOTS] = {NULL, NULL, NULL};
static U8     bDrvSysfs[MADBUS_NUMBER_SLOTS] = {0, 0, 0};
static U32    NumDrvrs = 0;

//This function implements a simulation of the equivalent pci function
//We save this pci-driver struct in a set to work w/ multiple client device drivrs
//
int pcisim_register_driver(struct pci_driver *pcidrvr)
{
	int rc = 0;
	//
    ASSERT((int)(pcidrvr != NULL));

	pcidrvr->driver.bus = &madbus_type;
	rc = driver_register(&pcidrvr->driver);
	if (rc)
	    {
	    PERR("pcisim_register_driver:driver_register returned (%d)\n", rc);
	    return rc;
	    }

    PciDrvrs[NumDrvrs] = pcidrvr;

    DrvrAttr.attr.name = pcidrvr->driver.name;
    //DrvrAttr.attr.owner = pcidrvr->driver.owner;
    //
	//rc = driver_create_file(&pcidrvr->driver, &DrvrAttr);
	if (rc != 0)
        {printk(KERN_DEBUG
                "pcisim_register_driver:driver_create_file returned (%d)\n", rc);}

    //bDrvSysfs[NumDrvrs] = (rc == 0);
    NumDrvrs++;
    PINFO("pcisim_register_driver... client driver=%s\n", pcidrvr->driver.name);

    //driver_create_file failure should not be fatal
    return 0;
}

//This function implements a simulation of the equivalent pci function
//We remove one pci device driver from the current set
//
void pcisim_unregister_driver(struct pci_driver *pcidrvr)
{
    register U32 j;

    ASSERT((int)(pcidrvr != NULL));
	PINFO("pcisim_unregister_driver... pcidrvr=%px\n", pcidrvr);

    for (j=0; j < NumDrvrs; j++)
        {
        if (pcidrvr == PciDrvrs[j])
            {
           	PDEBUG("pcisim_unregister_driver... client driver=%s\n",
                   pcidrvr->driver.name);

            //if (bDrvSysfs[j] != 0)
            //    {driver_remove_file(&pcidrvr->driver, &DrvrAttr);}

	        driver_unregister(&pcidrvr->driver);
            PciDrvrs[j] = PciDrvrs[NumDrvrs-1];
            bDrvSysfs[j] = bDrvSysfs[NumDrvrs-1];
            PciDrvrs[NumDrvrs-1] = NULL;
            NumDrvrs--;
            break;
            }
        }

    return;
}
//
EXPORT_SYMBOL(pcisim_register_driver);
EXPORT_SYMBOL(pcisim_unregister_driver);

//These structures are useful to spoof the device_bound_to_driver state 
//in simulation mode
static struct klist maddev_klist;
//static struct list_head empty_list = {&empty_list, &empty_list};

static struct device_private maddev_priv_data = 
{
    .knode_driver   = {.n_klist = &maddev_klist,}, //avoid a null-pntr at unregister

    //set the list to empty by pointers to itself next,prev
    .deferred_probe = {&maddev_priv_data.deferred_probe,
                       &maddev_priv_data.deferred_probe,},
    .dead = 0,
};

int sim_register_device(struct device* pDevice)
{
    PINFO("sim_register_device... pDevice=%px\n", pDevice);

    pDevice->bus     = &madbus_type;
    pDevice->parent  = &madbus_dev;
    pDevice->release = madbus_release;
    pDevice->p       = &maddev_priv_data;
    //
    //return device_register(pDevice);
    //device_initialize(pDevice);
    //return device_add(pDevice);
    return 0;
}
//
static void sim_device_unregister(struct device* pDevice)
{
    PINFO("sim_device_unregister... pDevice=%px\n", pDevice);

    //device_del(pDevice);
    //put_device(NULL);
}

void sim_unregister_device(struct device* pDevice)
{
    PINFO("sim_unregister_device... pDevice=%px\n", pDevice);
    pDevice->bus = NULL;
    //device_unregister(pDevice);
    sim_device_unregister(pDevice);
}
//
EXPORT_SYMBOL(sim_register_device);
EXPORT_SYMBOL(sim_unregister_device);

// This function implements a simulation of the equivalent pci function
// It returns an initialized pci_dev from the next available bus slot
//
struct pci_dev*
pcisim_get_device(unsigned int vendor, unsigned int pci_devid, struct pci_dev *start)
{
    PMADBUSOBJ pmadbusobj; 
    int        indx = 0;

    if ((vendor != MAD_PCI_VENDOR_ID) ||
        (pci_devid < MAD_PCI_BASE_DEVICE_ID) || (pci_devid > MAD_PCI_MAX_DEVICE_ID))
        {
       	printk(KERN_DEBUG
               "pcisim_get_device vendor:device_id mismatch... rc=-EINVAL\n");
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
    //
    return madbus_setup_pci_device((indx+1), pci_devid);
}
//
EXPORT_SYMBOL(pcisim_get_device);

//This function implements a simulation of the equivalent pci function
//
int pcisim_enable_device(struct pci_dev* pPciDev)
{
   	struct madbus_object *mbobj;

    if (pPciDev == NULL)
        {return -ENODEV;}

    mbobj = container_of(pPciDev, struct madbus_object, pcidev);
    PINFO("pcisim_enable_device... dev#=%d mbobj=%px pcidev=%px\n",
		   (int)mbobj->devnum, mbobj, &mbobj->pcidev);

    return 0;
}
//
EXPORT_SYMBOL(pcisim_enable_device);

//This function implements a simulation of the equivalent pci function
int pcisim_disable_device(struct pci_dev* pPciDev)
{
   	struct madbus_object *mbobj;

    if (pPciDev == NULL)
        {return -ENODEV;}

    mbobj = container_of(pPciDev, struct madbus_object, pcidev);
    PINFO("pcisim_disable_device... dev#=%d mbobj=%px pcidev=%px\n",
		  (int)mbobj->devnum, mbobj, &mbobj->pcidev);

    return 0;
}
//
EXPORT_SYMBOL(pcisim_disable_device);

//This function implements a simulation of the equivalent pci function
int pcisim_enable_msi_block(struct pci_dev* pPciDev, int num)
{
   	struct madbus_object *mbobj;

   if (pPciDev == NULL)
        {return -ENODEV;}

    if (num > 8)
        {return -EINVAL;}

    mbobj = container_of(pPciDev, struct madbus_object, pcidev);
    PINFO("pcisim_enable_msi_block... dev#=%d mbobj=%px pcidev=%px\n",
		  (int)mbobj->devnum, mbobj, &mbobj->pcidev);

    return 0;
}
//
EXPORT_SYMBOL(pcisim_enable_msi_block);

//This function implements a simulation of the equivalent pci function
void pcisim_disable_msi(struct pci_dev* pPciDev)
{
   	struct madbus_object *mbobj;

   if (pPciDev == NULL)
        {return;}

    mbobj = container_of(pPciDev, struct madbus_object, pcidev);
    PINFO("pcisim_disable_msi... dev#=%d mbobj=%px pcidev=%px\n",
		  (int)mbobj->devnum, mbobj, &mbobj->pcidev);

    return;
}
//
EXPORT_SYMBOL(pcisim_disable_msi);

//This function implements a simulation of the equivalent pci function
int pcisim_read_config_byte(const struct pci_dev* pPciDev, int where, u8 *val)
{
	struct madbus_object *mbobj = container_of(pPciDev, struct madbus_object, pcidev);
	U8*        pPciConfig;

	if (mbobj == NULL)
		{return -ENODEV;}

	PDEBUG("pcisim_read_config_byte... dev#=%d mbobj=%px pcidev=%px\n",
		   (int)mbobj->devnum, mbobj, &mbobj->pcidev);

    if ((where > MAD_PCI_CFG_SPACE_SIZE) || (where < 0))
	    {return -EINVAL;}

	pPciConfig = mbobj->PciConfig;
	*val = pPciConfig[where];

    return 0;
}

//This function implements a simulation of the equivalent pci function
int pcisim_read_config_word(const struct pci_dev* pPciDev, int where, u16 *val)
{
	struct madbus_object *mbobj = container_of(pPciDev, struct madbus_object, pcidev);
	U8*        pPciConfig;
    u16*       pPciCnfg16;

	if (mbobj == NULL)
		{return -ENODEV;}

	if ((where > MAD_PCI_CFG_SPACE_SIZE) || (where < 0))
	    {return -EINVAL;}

	PDEBUG("pcisim_read_config_word... dev#=%d mbobj=%px pcidev=%px\n",
		   (int)mbobj->devnum, mbobj, &mbobj->pcidev);

	pPciConfig = mbobj->PciConfig;
    pPciCnfg16 = (U16*)&(pPciConfig[where]);
	*val = *pPciCnfg16;

    return 0;
}

//This function implements a simulation of the equivalent pci function
int pcisim_read_config_dword(const struct pci_dev *pPciDev, int offset, U32 *val)
{
	struct madbus_object *mbobj = 
    container_of(pPciDev, struct madbus_object, pcidev);
	U8*        pPciConfig;
    U32*       pPciCnfg32;

	if (mbobj == NULL)
		{return -ENODEV;}

	if ((offset > MAD_PCI_CFG_SPACE_SIZE) || (offset < 0))
	    {return -EINVAL;}

	PDEBUG("pcisim_read_config_word... dev#=%d mbobj=%px pcidev=%px offset=%d\n",
		   (int)mbobj->devnum, mbobj, &mbobj->pcidev, offset);

	pPciConfig = mbobj->PciConfig;
    pPciCnfg32 = (U32*)&(pPciConfig[offset]);
	*val = *pPciCnfg32;

    return 0;
}
//
EXPORT_SYMBOL(pcisim_read_config_byte);
EXPORT_SYMBOL(pcisim_read_config_word);
EXPORT_SYMBOL(pcisim_read_config_dword);

//This function implements a simulation of the equivalent pci function
int  pcisim_request_region(const struct pci_dev *pdev, int bar, char* resname)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

    return 0;
}

//This function implements a simulation of the equivalent pci function
void   pcisim_release_region(const struct pci_dev *pdev, int bar)
{
    /*PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    /.*if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

    if (bar > 0)
        {return -EINVAL;}*/

    return;
}
//
EXPORT_SYMBOL(pcisim_request_region);  
EXPORT_SYMBOL(pcisim_release_region);  

//This function implements a simulation of the equivalent pci function
phys_addr_t pcisim_resource_start(const struct pci_dev *pdev, int bar)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

   if (bar > 0)
        {return -EINVAL;}

    return pmadbusobj->MadDevPA;
}
//
EXPORT_SYMBOL(pcisim_resource_start);  

//This function implements a simulation of the equivalent pci function
U64 pcisim_resource_end(const struct pci_dev *pdev, int bar)
{
   PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
   if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

   if (bar > 0)
        {return -EINVAL;}

    return (pmadbusobj->MadDevPA + MAD_DEVICE_MEM_SIZE_NODATA - 1);
}
//
EXPORT_SYMBOL(pcisim_resource_end);  

//This function implements a simulation of the equivalent pci function
U32 pcisim_resource_len(const struct pci_dev *pdev, int bar)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

   if (bar > 0)
        {return -EINVAL;}

    return MAD_DEVICE_MEM_SIZE_NODATA;
}
//
EXPORT_SYMBOL(pcisim_resource_len);  

//This function implements a simulation of the equivalent pci function
U32 pcisim_resource_flags(const struct pci_dev *pdev, int bar)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, pcidev);
    if (pmadbusobj->pci_devid == 0)
        {return -ENODEV;}

   if (bar > 0)
        {return -EINVAL;}

    return IORESOURCE_MEM;
}
//
EXPORT_SYMBOL(pcisim_resource_flags);  

//This function implements a simulation of the kernel/pcicore request_irq function
int  sim_request_irq(unsigned int irq, void* isrfunxn,
                     U32 flags, const char* dev_name, void* dev_id)
{
    U32 devnum;
    PMADBUSOBJ pmadbusobj;
    int isrdx = 0;
    U8  bMSI;

    if (dev_id == NULL)
        {return -ENODEV;}

    devnum = *(U32*)dev_id; //assumes device# is at offset zero
    pmadbusobj = &madbus_objects[devnum];

    bMSI = pmadbusobj->pcidev.msi_cap;
    if(!bMSI)
        { //Legacy INT devices have one ISR
        if (irq != ((U32)(madbus_base_irq + devnum)))
            {return -EINVAL;}

        pmadbusobj->isrfn[0] = isrfunxn;
        pmadbusobj->irq[0] = irq;
        }
    else 
        { //MSI-capable devices may have 1..8 ISRs
        isrdx = (int)(irq - (U32)(madbus_base_irq + devnum));
        if ((isrdx < 0) || (isrdx > 7))
           {return -EINVAL;}

        pmadbusobj->isrfn[isrdx] = isrfunxn;
        pmadbusobj->irq[isrdx] = irq;
        }

    //The requestor is granted the irq
    return 0;       
}
//
//This function implements a simulation of the kernel/pcicore free_irq function
int sim_free_irq(unsigned int irq, void* dev_id)
{
    U32 devnum;

    if (dev_id == NULL)
        {return -ENODEV;}

    devnum = *(U32*)dev_id; //assumes device# is at offset zero
    //if ((irq != ((U32)(madbus_base_irq + devnum)))
    //    {return -EINVAL;}

    return 0;       
}
//
EXPORT_SYMBOL(sim_request_irq);
EXPORT_SYMBOL(sim_free_irq);

//This function provides a pointer to a parm exchange area so that
//the client driver can operate in simulation mode
PMAD_SIMULATOR_PARMS madbus_xchange_parms(int num)
{
	PMADBUSOBJ pmadbusobj = &madbus_objects[num];
    if (pmadbusobj->pmaddevice == NULL) //We didn't get ram
        {return NULL;}

    //if (num==3) //Spoofing that we didn't get ram
    //    {return NULL;}

    pmadbusobj->SimParms.pmadbusobj = pmadbusobj;
    pmadbusobj->SimParms.pcomplete_simulated_io = madsim_complete_simulated_io;

    PDEBUG("madbus_xchange_parms... dev#=%d pmbobj=%px pCmpltSimIo=%px\n",
           (int)pmadbusobj->devnum, pmadbusobj, 
           pmadbusobj->SimParms.pcomplete_simulated_io); /* */

	return &pmadbusobj->SimParms;
}
//
EXPORT_SYMBOL(madbus_xchange_parms);

//This function simulates a PCI device hotplug
static int madbus_hotplug(U32 indx, U16 pci_devid)
{
    register U32 j;
    //
    PMADBUSOBJ pmbobj_hpl;
    struct pci_driver* pPciDrvr;
    struct pci_dev*    pPciDev = NULL;
    int                rc = -EUNATCH; //No 'protocol' driver attached until we find one

    PINFO("madbus_hotplug... dev#=%d pci_devid=x%X\n", (int)indx, pci_devid);

    if ((indx < 1) || (indx > madbus_nbr_slots))
        {return -EBADSLT;} //Slot# out of bounds 

    pmbobj_hpl = &madbus_objects[indx];
    if (pmbobj_hpl->pci_devid != 0)
       {return -EEXIST;} //This bus slot is in use;

    pPciDev = madbus_setup_pci_device(indx, pci_devid);
    if ((pPciDev == NULL) || (IS_ERR(pPciDev)))
        {return -EFAULT;}

    ASSERT((int)(pPciDev == &pmbobj_hpl->pcidev));

    //Find the correct device driver from the set of known client drivers
    //Calling every drivers' probe function until a match
    for (j = 0; j < NumDrvrs; j++)
        {
        pPciDrvr = PciDrvrs[j];
        if (pPciDrvr == NULL)
            {continue;}

        ASSERT((int)(pPciDrvr->probe != NULL));
        if (pPciDrvr->probe == NULL)
            {continue;}

        rc = pPciDrvr->probe(&pmbobj_hpl->pcidev, pPciDrvr->id_table);
        if (rc == 0)
            {break;}
        }

    if (rc == 0)
        {ASSERT((int)(pmbobj_hpl->pcidev.driver != NULL));}
    else
        //Mark this slot as free
        {pmbobj_hpl->pci_devid = 0;}

    //The hotplug result is whatever the last probe function returned
    //if any probe was invoked
    return rc;
}

//This function simulates a PCI device hot unplug
static int madbus_hotunplug(U32 indx)
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

    pPciDrvr = pmbobj_hpl->pcidev.driver; 
    if (pPciDrvr == NULL)
        {return -EUNATCH;} //No 'protocol' driver attached 

    if (pPciDrvr->remove == NULL)
       {return -EFAULT;}

    /*rc =*/ pPciDrvr->remove(&pmbobj_hpl->pcidev);
    rc = 0;

    pmbobj_hpl->pci_devid = 0;

    //The final result of the unplug is ok if any remove was invoked
    return 0;
}

// This function sets up and initializes one pci device
// whether found on the bus or hotplugged
static struct pci_dev* madbus_setup_pci_device(U32 indx, U16 pci_devid)
{
    int rc;
    PMADBUSOBJ pmadbusobj = &madbus_objects[indx];
    pmadbusobj->pci_devid = pci_devid;

    PINFO("madbus_setup_pci_device... dev#=%d mbobj=%px pcidevid=x%X pcidev=%px\n",
		  (int)indx, pmadbusobj, pci_devid, &(pmadbusobj->pcidev));

    memset(&pmadbusobj->pcidev, 0x00, sizeof(struct pci_dev));
    pmadbusobj->pci_devid = pci_devid;
    //
    rc = madbus_set_pci_config(pmadbusobj);
    if (rc != 0)
        {
   	    PERR("madbus_setup_pci_device... dev#=%d rc=%d\n",
		     (int)pmadbusobj->devnum, rc);
        pmadbusobj->pci_devid = 0;
        return ERR_PTR(rc);
        }

    madbus_init_pcidev(pmadbusobj);

    return &(pmadbusobj->pcidev);
}

//This function returns a kernel virtual address whether or not the
//physical address is currently mapped into kernel virt addr space
static void* Get_KVA(phys_addr_t PhysAddr, struct page** ppPgStr)
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

    PDEBUG("Get_KVA... PA=x%X pfn=x%X pPgStr=%px kva=%px remap=%d\n",
           (unsigned int)PhysAddr, (unsigned long long)pfn,
           pPgStr, pvoid, (int)bKMAP);
    ASSERT((int)(pvoid != NULL));
    BUG_ON(pvoid == NULL);

    return pvoid;
}

static char DfltPciConfig[MAD_PCI_CFG_SPACE_SIZE];
//
//Populate a template pci config space with generic data
void   SimInitPciConfig(char* PciCnfgSpace)
{
U8*    pChar  = NULL;
U16*   pShort = NULL;
U32*   pLong = NULL;

    ASSERT((int)(PciCnfgSpace != NULL));
    memset(PciCnfgSpace, 0x00, MAD_PCI_CFG_SPACE_SIZE);

 	pShort = (U16*)&PciCnfgSpace[PCI_VENDOR_ID];
	*pShort = MAD_PCI_VENDOR_ID;
    //
	pShort++;
    *pShort = MAD_PCI_BASE_DEVICE_ID;
	
	pShort++; //Command
    pShort++; //Status 
	pShort++; //Class_Revision
	pShort++; //Class_Device
    *pShort = 5; //memory controller

    pLong = (U32*)&PciCnfgSpace[PCI_BASE_ADDRESS_0];
    *pLong = 0xFFFF0000;
    //
	pLong++; //PCI_BASE_ADDRESS_1
    *pLong = 0xFFFF1000;
    //
	pLong++; //PCI_BASE_ADDRESS_2
    *pLong = 0xFFFF2000;
    //
	pLong++; //PCI_BASE_ADDRESS_3
    *pLong = 0xFFFF3000;
    //
	pLong++; //PCI_BASE_ADDRESS_4
    *pLong = 0xFFFF4000;
    //
	pLong++; //PCI_BASE_ADDRESS_5
    *pLong = 0xFFFF5000;

	pShort = (U16*)&PciCnfgSpace[PCI_SUBSYSTEM_VENDOR_ID];
	*pShort = MAD_PCI_VENDOR_ID;
    //
	pShort++; //PCI_SUBSYSTEM__ID
    *pShort = MAD_PCI_BASE_DEVICE_ID;

	pChar = (U8*)&PciCnfgSpace[PCI_INTERRUPT_LINE];
    *pChar = 0x0A;
	//
	pChar++; //PCI_INTERRUPT_PIN
	*pChar = 0x01;

	return;
}

//This function creates a custom PCI config space for a device
//starting from an initialized template
static int madbus_set_pci_config(PMADBUSOBJ pmadbusobj)
{
    U8         bMSI = 0;
               //((pmadbusobj->pci_devid % 2) == 0); //Even# dev_ids are MSI-capable
    //
	U8*        pPciCnfgParm;
    U16*       pPciCnfgU16;
    U64*       pPciCnfgU64;
    U32*       pPciCnfgU32;

    ASSERT((int)(pmadbusobj != NULL));

	if (pmadbusobj->pmaddevice == NULL)
		{return -ENODEV;}

   	pPciCnfgParm = (U8*)(pmadbusobj->PciConfig);
	memcpy(pPciCnfgParm, DfltPciConfig, MAD_PCI_CFG_SPACE_SIZE);
	//
    pPciCnfgU16 = (U16*)&(pmadbusobj->PciConfig[PCI_DEVICE_ID]);
    *pPciCnfgU16 = pmadbusobj->pci_devid;
    pPciCnfgU16 = (U16*)&(pmadbusobj->PciConfig[PCI_SUBSYSTEM_ID]);
    *pPciCnfgU16 = pmadbusobj->pci_devid;
    //
    pPciCnfgU64  = (U64*)&pPciCnfgParm[PCI_BASE_ADDRESS_0];
    ASSERT((int)(((u64)pPciCnfgU64 % 8) == 0));
    *pPciCnfgU64 = pmadbusobj->MadDevPA;

    pPciCnfgU64++; //PCI_BASE_ADDRESS_2
    *pPciCnfgU64 = (pmadbusobj->MadDevPA + MAD_MAPD_READ_OFFSET);

    pPciCnfgU64++; //PCI_BASE_ADDRESS_4
    *pPciCnfgU64 = (pmadbusobj->MadDevPA + MAD_MAPD_WRITE_OFFSET);

    //pPciCnfgU32++; //PCI_BASE_ADDRESS_3
    //*pPciCnfgU32 = 0;
    //pPciCnfgU32++; //PCI_BASE_ADDRESS_4
    //*pPciCnfgU32 = 0;
    //pPciCnfgU32++; //PCI_BASE_ADDRESS_5
    //*pPciCnfgU32 = 0;

    pPciCnfgParm[PCI_INTERRUPT_LINE] = (U8)(madbus_base_irq + pmadbusobj->devnum);
    pPciCnfgParm[PCI_INTERRUPT_PIN] = pmadbusobj->devnum;

    //We treat MSI capability as a yes/no
    pPciCnfgParm[PCI_CAPABILITY_LIST + PCI_CAP_ID_MSI] = bMSI;

    //Put the pointer to the simulation parms at a known location
    //
    pPciCnfgU32 = (U32*)&pPciCnfgParm[MAD_PCI_VENDOR_OFFSET];
    *pPciCnfgU32 = (U32)&pmadbusobj->SimParms;

    PINFO("madbus_set_pci_config... dev#=%d bMSI=%d PciCnfgBase0_loc,val=%px,x%X\n",
          (int)pmadbusobj->devnum, bMSI, &pPciCnfgParm[PCI_BASE_ADDRESS_0],
          (U64)(pPciCnfgParm[PCI_BASE_ADDRESS_0]));

    return 0;
}
//
//This function creates a pci dev struct to provide to the pci device driver
static void madbus_init_pcidev(PMADBUSOBJ pmadbusobj)
{
    struct pci_dev* pPciDev; 
	U8*        pPciCnfgParm; 

    ASSERT((int)(pmadbusobj != NULL));
    pPciDev = &pmadbusobj->pcidev;

    PINFO("madbus_init_pcidev... dev#=%d mbobj=%px\n",
          (int)pmadbusobj->devnum, pmadbusobj);

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_VENDOR_ID]);
    pPciDev->vendor = *(U16*)pPciCnfgParm;

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_DEVICE_ID]);
    pPciDev->device = *(U16*)pPciCnfgParm;

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_SUBSYSTEM_VENDOR_ID]);
    pPciDev->subsystem_vendor = *(U16*)pPciCnfgParm;

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_SUBSYSTEM_ID]);
    pPciDev->subsystem_device = *(U16*)pPciCnfgParm;

    pPciDev->cfg_size = MAD_PCI_CFG_SPACE_SIZE;

    pPciCnfgParm = &(pmadbusobj->PciConfig[PCI_INTERRUPT_LINE]);
    pPciDev->irq = (int)*pPciCnfgParm;

    pPciDev->msi_cap = pmadbusobj->PciConfig[PCI_CAPABILITY_LIST + PCI_CAP_ID_MSI];

    //We don't populate the pci slot struct - just use the pntr as a slot#
    pPciDev->slot = (struct pci_slot *)pmadbusobj->devnum;

    pPciDev->driver = NULL;//Should be assigned by the claiming device driver 

    return;
}
#endif //_DEVICE_DRIVER_MAIN_
#endif //_SIM_DRIVER_
//
#endif  //_SIM_DRVR_LIB_
