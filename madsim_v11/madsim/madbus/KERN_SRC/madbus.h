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

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/fs.h>		/* everything... */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
//
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/page.h>	
//
#ifdef _SIM_DRIVER_
#define DRIVER_NAME "madbus.ko"
#endif

#include "../../include/maddefs.h"
#include "../../include/madkonsts.h"
#include "../../include/madbusioctls.h"

#define  MADBUSOBJNAME   "madbusobjX"
#define  MBDEVNUMDX      9 //......^

#define MADBUS_MAJOR_OBJECT_NAME  "madbus_object"
#define MADBUS_NBR_DEVS           MAD_NBR_DEVS
#define MADBUS_BASE_IRQ           30

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
	void*       pmaddevobj;
    struct      pci_dev*  pPcidev;
    PMADREGS    pmadregs;
};
//
typedef struct mad_simulator_parms *PMAD_SIMULATOR_PARMS;

// The device context structure for the bus-level char-mode device
struct madbus_object
{
	U32        devnum;
    u32        dev_iotag;
    U32        junk; //8-byte align below
    char       PciConfig[MAD_PCI_CFG_SPACE_SIZE];

	//struct     mad_driver *driver;
	//struct     semaphore sem;     /* mutual exclusion semaphore     */
	//spinlock_t devlock;
    struct page* pPage;
	phys_addr_t  MadDevPA;
    PMADREGS    pmaddevice;
    //
    irq_handler_t  isrfn[MAD_NUM_MSI_IRQS+1];
    int            irq[MAD_NUM_MSI_IRQS+1];

    //The device to be exposed in SysFs
    U32        bRegstrd;
    struct     device  sysfs_dev;

    //The PCI sumulation device
    U16        pci_devid;
    struct     pci_dev pcidev;

	struct     cdev cdev_str;
    char       dummy[sizeof(struct list_head)];
	struct     task_struct *pThread;
	struct     mad_simulator_parms SimParms;
   	char       *name;
};
//
typedef struct madbus_object MADBUSOBJ, *PMADBUSOBJ;

#define to_madbus_object(dev) container_of(dev, struct madbus_object, dev);

int madbus_setup_device(PMADBUSOBJ pmaddbusobj, u32 indx);
extern int register_mad_device(struct madbus_object *);
extern void unregister_mad_device(struct madbus_object *);
extern int madbus_create_thread(PMADBUSOBJ pmadbusobj);
extern int madbus_dev_thread(void* pvoid);
void mbdt_process_bufrd_io(PMADBUSOBJ pmadbusobj, bool write, u32 msidx);
void mbdt_process_cache_io(PMADBUSOBJ pmadbusobj, bool write, u32 msidx);
void mbdt_process_align_cache(PMADBUSOBJ pmadbusobj, bool write, u32 msidx);
void mbdt_process_dma_io(PMADBUSOBJ pmadbusobj, bool write, u32 msidx);
void mbdt_process_sgdma(PMADREGS pmaddevice, bool bWrite);
//
void madsim_complete_simulated_io(void* vpmadbusobj, PMADREGS pmadregs);
void madsim_complete_xfer_one_dma_element(PMADBUSOBJ pmadbusobj, 
                                                 PMAD_DMA_CHAIN_ELEMENT pSgDmaElement);
void madsim_complete_simulated_sgdma(PMADBUSOBJ pmadbusobj, PMADREGS pmadregs);
#ifdef _SIM_DRIVER_
#include "../../include/simdrvrlib.h"
#endif
//


