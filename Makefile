export KERNELDIR = ~/m2/nmv/linux-4.2.3
export SHNAME = shellghoumi

SRC = src
MODSRC = $(SRC)/shmodule
USRSRC = $(SRC)/$(SHNAME)
TSTSRC = $(SRC)/test

all: module usrshell test

module:
	cd $(MODSRC) && make

usrshell:
	cd $(USRSRC) && make

test:
	cd $(TSTSRC) && make

clean:
	cd $(MODSRC) && make clean
	cd $(USRSRC) && make clean
	cd $(TSTSRC) && make clean

.PHONY: all module usrshell test clean
