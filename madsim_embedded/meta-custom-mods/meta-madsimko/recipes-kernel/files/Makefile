obj-m := madbus.o maddevc.o

madbus-objs  := madbusmain.o mbdevthread.o
maddevc-objs := maddevcmain.o maddevcio.o
###maddevb-objs := maddevbmain.o maddevbio.o maddevb_blk_utils.o maddevb_zoned.o

ccflags-y += -O0 -g #
ccflags-y += -Wno-declaration-after-statement

UBSAN_SANITIZE := y

SRC := $(CURDIR)

all: modules

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean
#	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
#	rm -f Module.markers Module.symvers modules.order
#	rm -rf .tmp_versions Modules.symvers
