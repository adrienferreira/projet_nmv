.PHONY: all 
#.SECONDARY:

ifneq ($(KERNELRELEASE),)

  obj-m := killmodule.o

else
	
  #KERNELDIR ?= /lib/modules/$(shell uname -r)/build
  KERNELDIR ?=/home/bernadette/Logiciels/NMV/linux-4.2.3/
  PWD := $(shell pwd)

all :
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

endif

