TOOLPREFIX = riscv64-unknown-elf-

platform = K210

LD = $(TOOLPREFIX)ld
CXX = $(TOOLPREFIX)g++
CC = $(TOOLPREFIX)gcc
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
T = ../target

DFLAGS = -D

OBJ_LINK_LIST:=$(CRTI_OBJ) $(CRTBEGIN_OBJ) $(OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)
INTERNAL_OBJS:=$(CRTI_OBJ) $(OBJS) $(CRTN_OBJ)

FLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -g
FLAGS += -MD
FLAGS += -mcmodel=medany
FLAGS += -ffreestanding -fno-common -nostdlib -mno-relax -fcheck-new
FLAGS += -I./
FLAGS += -fno-stack-protector
FLAGS += -Wno-sign-compare

FLAGS += -DK210
link = kernel.ld

LDFLAGS = -z max-page-size=4096
CXXFLAGS = -std=c++11
CXXFLAGS += -fno-rtti
CXXFLAGS += -fno-exceptions
CXXFLAGS += $(FLAGS)
CFLAGS = $(FLAGS)

OBJS = \
	init/entry.o 			\
	init/main.o 		 	\
	trap/kernelvec.o 		\
	driver/uart.o 			\
	


-include */*.d

CRTI_OBJ=init/crti.o 
CRTBEGIN_OBJ:=$(shell $(CXX) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CXX) $(CFLAGS) -print-file-name=crtend.o)
CRTN_OB=init/crtn.o 

OBJ_LINK_LIST := $(OBJS)

kernelImg = $T/kernel.bin
k210 = $T/k210.bin
RUSTSBI = ../rustsbi/sbi-k210

kernel : $(link) $(OBJ_LINK_LIST)
	$(LD) $(LDFLAGS) -T $(link) -o $T/kernel $(OBJ_LINK_LIST) -nostdlib
	$(OBJDUMP) -S $T/kernel > $T/kernel.asm
	@$(OBJDUMP) -t $T/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $T/kernel.sym
	$(OBJCOPY) $T/kernel --strip-all -O binary $(kernelImg)
	$(OBJCOPY) $(RUSTSBI) --strip-all -O binary $(k210)
	dd if=$(kernelImg) of=$(k210) bs=128k seek=1
	cp $(k210) ../k210.bin

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg *.d \
	*/*.o */*.d */*.asm */*.sym \
	*/*/*.o */*/*.d */*/*.asm */*/*.sym \
	kernel fs.img *.asm \
