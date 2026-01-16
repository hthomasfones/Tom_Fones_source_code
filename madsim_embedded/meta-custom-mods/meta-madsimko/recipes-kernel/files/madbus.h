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
/*  Module NAME : madbuss.h                                                    */
/*                                                                             */
/*  DESCRIPTION : Definitions for the Model-Abstract Device bus driver         */
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
/* This source file was derived from source code developed by                  */
/* ALESSANDRO RUBINI and JONATHAN CORBET.                                      */
/* Copyright (C) 2001 O'REILLY & ASSOCIATES -- appearing in the book:          */
/* "LINUX DEVICE DRIVERS" by Rubini and Corbet,                                */
/* published by O'Reilly & Associates.                                         */
/* No warranty is attached to the original source code.                        */
/*                                                                             */
/*                                                                             */
/* $Id: madbus.h, v 1.0 2021/01/01 00:00:00 htf $                              */
/*                                                                             */
/*******************************************************************************/

#define _KERNEL_MODULE_
#include <linux/kconfig.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/fs.h>		/* everything... */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/uaccess.h> 
#include <asm/page.h>
#include <linux/types.h>
#include <linux/byteorder/generic.h>   // cpu_to_le16/32/64
#include <asm/unaligned.h>             // put_unaligned_leXX	
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direct.h>

#ifdef _SIM_DRIVER_
#define DRIVER_NAME "madbus.ko"
#endif

#define _MAD_SIMULATION_MODE_
#include "maddefs.h"
#include "madkonsts.h"
#include "madbusioctls.h"

#define  MADBUSOBJNAME   "madbusobjX"
#define  MBDEVNUMDX      9 //......^

#define MADBUS_MAJOR_OBJECT_NAME  "madbus_object"
#define MADBUS_NBR_DEVS           MAD_NBR_DEVS
#define MADBUS_BASE_IRQ           30

typedef enum  _MADBUS_ALLOC_ORDER_ 
        {eKmallocBytes = MAD_PAGE_ORDER_KMALLOC_BYTES,//What we can get w/ kmalloc...  2MB
         eAllocPages   = MAD_PAGE_ORDER_ALLOC_PAGES, //What we can get w/ alloc_pages... 4MB  
         eCmaAlloc     = MAD_PAGE_ORDER_CMA_ALLOC , //Contiguous Memory Allocator... 8MB+      
         eDtbSharedMem = MAD_PAGE_ORDER_DTB_ALLOC_SHARED,  //CMA of Device-Tree reserved memory 16MB
         eDtbPrivMem   = MAD_PAGE_ORDER_DTB_ALLOC_PRIV  //CMA of Device-Tree reserved memory 32MB
        }  MADBUS_ALLOC_ORDER;

//A parm area to exchange information between the simulator & device driver(s)
//A device driver only cares about this in simulation mode
//
struct mad_simulator_parms
{
    void*       pmadbusobj;
    void        (*pcomplete_simulated_io)(void* pmadbusobj, PMADREGS pmaddevice); 

    void*       pInBufr;
    void*       pOutBufr;
    //
	spinlock_t* pdevlock;
	void*       pmaddev;
    struct      pci_dev*  pPcidev;
    __iomem MADREGS  *pmadregs;
};
//
typedef struct mad_simulator_parms *PMAD_SIMULATOR_PARMS;

// The device context structure for the bus-level char-mode device
struct madbus_object
{
	U32        devnum;
    U32        dev_iotag;
    //U32        junk; //8-byte align below
    u8         PciConfig[MAD_PCI_CFG_SPACE_SIZE];
    struct     mad_simulator_parms SimParms;
    //u32        sim_cookie;  
	//struct     mad_driver *driver;
	//struct     semaphore sem;     /* mutual exclusion semaphore     */
	//spinlock_t devlock;
    size_t       alloc_size;
    uint32_t     numpages;
    struct page* pPage;
    dma_addr_t  hDMA;
	phys_addr_t  MadDevPA;
    __iomem MADREGS  *pmadregs; 
    struct      pci_slot pciSlot;
    //
    void*          isr_devobj;
    irq_handler_t  isrfn[MAD_NUM_MSI_IRQS+1];
    int            irq[MAD_NUM_MSI_IRQS+1];

    u32        bHP;
    //The device to be exposed in SysFs
    U32        bRegstrd;
    struct     device  sysfs_dev;

    //The PCI sumulation device
    U16        pci_devid;
    struct     pci_dev pcidev;

	struct     cdev cdev_str;
    char       dummy[sizeof(struct list_head)];
	struct     task_struct *pThread;
	
   	char       *name;
};
//
typedef struct madbus_object MADBUSOBJ, *PMADBUSOBJ;

#define to_madbus_object(dev) container_of(dev, struct madbus_object, dev);

int madbus_setup_devices(PMADBUSOBJ madbusobjs, int num_devices);
int madbus_setup_device(PMADBUSOBJ pmaddbusobj, u32 indx, u8 bHP);
extern void madbus_root_release(struct device *pdev);
extern void madbus_obj_release(struct device *pdev);
extern int register_mad_device(struct madbus_object *);
extern void unregister_mad_device(struct madbus_object *);
extern int madbus_create_thread(PMADBUSOBJ pmadbusobj);
extern int madbus_dev_thread(void* pvoid);
extern int madbus_free_device_memory(PMADBUSOBJ pmadbusobj);
void mbdt_process_bufrd_io(PMADBUSOBJ pmadbusobj, bool write, u32 msidx);
void mbdt_process_cache_io(PMADBUSOBJ pmadbusobj, bool write, u32 msidx);
void mbdt_process_align_cache(PMADBUSOBJ pmadbusobj, bool write, u32 msidx);
void mbdt_process_dma_io(PMADBUSOBJ pmadbusobj, bool write, u32 msidx);
void mbdt_process_sgdma(PMADREGS pmaddevice, bool bWrite);
//
void madsim_complete_simulated_io(void* vpmadbusobj, __iomem MADREGS *pmadregs);
void madsim_complete_xfer_one_dma_element(PMADBUSOBJ pmadbusobj, 
                                          __iomem MAD_DMA_CHAIN_ELEMENT *pSgDmaUnit);
void madsim_complete_simulated_sgdma(PMADBUSOBJ pmadbusobj, __iomem MADREGS *pmadregs);
#ifdef _SIM_DRIVER_
#include "simdrvrlib.h"
#endif
//
