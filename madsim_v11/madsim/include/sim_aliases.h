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
/*  Exe files   : maddevc.ko                                                   */ 
/*                                                                             */
/*  Module NAME : sim_aliases.h                                                */
/*                                                                             */
/*  DESCRIPTION : Properties & Definitions for the MAD character mode driver   */
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
/* $Id: sim_aliases.h, v 1.0 2021/01/01 00:00:00 htf $                         */
/*                                                                             */
/*******************************************************************************/

//Alias the pci functions to the simulator replacements
//
#undef   pci_register_driver
#define  pci_register_driver    pcisim_register_driver
#define  pci_unregister_driver  pcisim_unregister_driver
#define  register_device        sim_register_device
#define  unregister_device      sim_unregister_device
#define  pci_get_device         pcisim_get_device
#define  pci_read_config_byte   pcisim_read_config_byte
#define  pci_read_config_word   pcisim_read_config_word
#define  pci_read_config_dword  pcisim_read_config_dword

#define  pci_write_config_byte  pcisim_write_config_byte
#define  pci_write_config_word  pcisim_write_config_word
#define  pci_write_config_dword pcisim_write_config_dword

#define  pci_request_region     pcisim_request_region
#define  pci_release_region     pcisim_release_region

#undef   pci_resource_start
#define  pci_resource_start     pcisim_resource_start

#undef   pci_resource_end
#define  pci_resource_end       pcisim_resource_end 

#undef   pci_resource_len
#define  pci_resource_len       pcisim_resource_len

#undef   pci_resource_flags
#define  pci_resource_flags     pcisim_resource_flags

#define  pci_enable_device      pcisim_enable_device
#define  pci_disable_device     pcisim_disable_device
#define  pci_enable_msi_block   pcisim_enable_msi_block
#define  pci_disable_msi        pcisim_disable_msi    

#define  request_irq            sim_request_irq
#define  free_irq               sim_free_irq

//These functions external to the device driver should be found
//in the bus driver as EXPORT_SYMBOL(x) 
//and should be found in the bus drivers Module.symvers file
extern int    pcisim_register_driver(struct pci_driver *pcidrvr);
extern void   pcisim_unregister_driver(struct pci_driver *pcidrvr);
extern int    sim_register_device(struct device* pDevice);
extern void   sim_unregister_device(struct device* pDevice);
extern int    pcisim_enable_device(struct pci_dev* pcidev);
extern int    pcisim_disable_device(struct pci_dev* pcidev);
extern int    pcisim_enable_msi_block(struct pci_dev* pcidev, int num);
extern void   pcisim_disable_msi(struct pci_dev* pPciDev);

extern struct pci_dev*
pcisim_get_device(unsigned int vendor, unsigned int device, struct pci_dev *from);
extern int    pcisim_read_config_byte(const struct pci_dev *dev, int where, U8 *val);
extern int    pcisim_read_config_word(const struct pci_dev *dev, int where, U16 *val);
extern int    pcisim_read_config_dword(const struct pci_dev *dev, int where, U32 *val);
extern int    pcisim_request_region(const struct pci_dev *dev, int bar, char* resname);
extern void   pcisim_release_region(const struct pci_dev *dev, int bar);
extern U32    pcisim_resource_start(const struct pci_dev *dev, int bar);
extern U32    pcisim_resource_end(const struct pci_dev *dev, int bar);
extern U32    pcisim_resource_len(const struct pci_dev *dev, int bar);
extern U32    pcisim_resource_flags(const struct pci_dev *dev, int bar);

extern int    sim_request_irq(unsigned int irq, 
                              /*irqreturn_t (*isrfunxn)()*/ void* isrfunxn,
                              U32 flags, const char* dev_name, void* dev_id);
extern int    sim_free_irq(unsigned int irq, void* dev_id);

