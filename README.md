 Readme file for Madsim -- the Model Abstract Device Simulation package
 November 7, 2021

A. Introduction
   ============
   This software package is intended as a proof of concept of simulating memory-mapped 
   interrupting PCI devices in software on Linux. It includes the PCI bus and device
   simulating driver, character mode and block mode device drivers, and test programs
   and bash scripts. The drivers, test programs and custom linux image are built by
   Eclipse and Link+. 
   
   The simulated hardware device has four registers: interrupt-enable, interrupt-id,
   control and status. It is a very basic device which has two modes...
   buffered sequential io (char-mode) and random access sector-aligned dma io (block-mode).
   The definition of the device can be found in maddefs.h from lines 275 to 475.
   
   There is a Windows version of this simulation on the web site http://www.htfconsulting.com
   This web site provides a visual image of the device and how the test programs and device 
   drivers fit together, as well as a demo of how the windows package works.
   This Linux package does not provide any GUI programs, just command line programs.
   
   Soon there will be a VBox OVA file and a KVM Qcow2 file which are each the entire
   development VM for this package on the website http:/www.htfconsulting.com.                                                              	
   User-id:htfones, password:Admin123.   34GB *!*                                                        	
   
   Questions and helpful suggestions may be sent to Tom Fones at: htfones@proton.me


B. Environment
   ===========
   All of the development of this initial release was done on a VBox VM with 8GB of ram,
   four cpus, running Ubuntu 20.04 and a custom build of linux 5.7.19.
   The VBox VM manager itself was running on Ubuntu 20.04; Linux 5.8.x.
   It has also been exercised on QEMU-KVM on Ubuntu 20.04
   
   Please see the file dotconfig.txt for the precise Xconfig kernel build settings.
   
   
C. Exercising the simulation package
   =================================                               
   If you are working with the VM... please use User-id:htfones, password:Admin123.
   Go to the folder /home/htfones/eclipse-wkspc/madsim/scripts.
   The most useful bash scripts are: 
   maddriverstack.sh, madbufrdiotest.sh, madramdomiotest.sh, maddisktest.sh                                                            	

D. File Manifest
   =============
   madsim/ ... parent folder
     dotconfig.txt 
     The precise xconfig kernel build settings.
     
     page-writeback.c0, page-writeback.c: 
     Before and after source files to document a necessary kernel tweak to 5.7.19.
     
     contiguous.c0, contiguous.c: 
     Before and after source files to document a necessary kernel tweak for working with
     DMA-CMA memory for large allocations 
     
     include/ ... the set of include files for all drivers and test programs
                  source files: madapplib.h, madbusioctls.h, maddefs.h, maddevioctls.h
                                maddrvrdefs.c, maddrvrdefs.h, madkonsts.h, madlib.h
                                sim_aliases.h, simdrvrlib.h
      
     madbus/ ...  parent folder for the pci bus & device simulator
                  source files: madbusmain.c, mbdevthread.c, madbus.h 
                  Module.symvers - export_symbol entry points used to link the device drivers
                  
     maddevb/ ... parent folder for the block-mode device driver
                  source files: maddevbmain.c, maddevbio.c, maddevb_blk_utils.c maddevb.h
                  
     maddevc/ ... parent folder for the char-mode device driver
                  source files: maddevcmain.c, maddevcio.c, maddevc.h
                  
     madsimui/... parent folder for the simulator test program
                  source files: madsimui.cpp, madsimui.h
                  
     madtest/ ... parent folder for both device driver test programs source code
                  source files: madtest.cpp, madtest.h

     madtestb/ ... parent folder for the block-mode driver test programs
                    (not used)

     madtestc/ ... parent folder for the char-mode driver test programs
                   source files: madtestc.cpp
   
     scripts/ ...  a set of bash scripts to drive testing.        
         madenv.sh  ... sets up environment variables
         madinsmods.sh .... loads the device drivers and displays information
                            about drivers and devices
         maddriverstack.sh ... loads and unloads the driver stack   
         madbufrdiotest.sh ... runs the sequential buffered-io test on the 
                               char-mode devices     
         madrandomiotest.sh ... runs the random io test on the char-mode devices     
         madresults.sh ... presents results for the char-device tests
         maddisktest.sh ... runs the test for block-mode devices applying the following:
                            fdisk -l; mkfs; dosfsck; lsblk; mount; fio
         Various other shell scripts which have not been exercised lately.                   
                                                        
                    
E. Caveats
   =======
   1) As stated above, the test programs are not GUIs, just command line programs.
   
   2) The 'hardware device' does not implement any queueing.
      The device driver(s) complete one i/o before initiating the next.

   3) As stated above, this initial release was developed and tested on a VBox VM with
      8GB of ram, running Ubuntu 20.04 and a custom build of linux 5.7.19.
      It is now exercised on QEMU-KVM.
      The kernel build customization is documented in dotconfig.txt.      
      
   4) There is one kernel tweak which was necessary to get the block-dev driver working.
      A null pointer is weeded out in the source file page-writeback.c
      Please diff/compare page-writeback.c with page-writeback.c0
      
   5) The random i/o test causes "bad page state ... _refcount" errors im dmesg.
      This appears to be a regression in the kernel.   
      
   6) The simulator short-circuits some of the kobject management.
      It will crash and require rebooting aftera few runs of the same test.    
   
   7) The block-mode device is only exercised with an MSDOS disk format. 
   
   8) There are intermittent issues with working with more than one device at once.  
      
   9) There is no real-world Model-Abstract-Device.
      The device drivers have only been tested in simulation mode.   
      
   10) The extent of the device is limited by how much ram is allocated, using ...
      kmalloc, alloc_pages, or dma_alloc_contiguous (requires a special kernel build ...
      please diff/compare contiguous.c0 and contiguous.c) 
      With 8GB of ram we can kmalloc 2MB (order=9), or alloc_page 4MB (order=10). 
      
   11) Utilities possibly needing to be installed: tree, fio, dosfsck.     


F. Future development
   ==================
   Fixes to known issues.
   Any enhancement or adaptation that anyone is willing to pay for.
   
   Please look to GitHub:device_simulation for updates.

