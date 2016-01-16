.PHONY: all 
#.SECONDARY:

ifneq ($(KERNELRELEASE),)

  obj-m := shmod.o
  shmod-objs := return.o lsmod.o waits.o kill.o meminfo.o shmodule.o

else

  KERNELDIR ?= /lib/modules/$(shell uname -r)/build
  #KERNELDIR ?=/home/bernadette/Logiciels/NMV/linux-4.2.3/
  PWD := $(shell pwd)

all :
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

endif

