COMPILERPREFIX = riscv64-unknown-elf-
TOOLPREFIX = riscv64-linux-gnu-

platform := k210

OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
K = kern
T = target
U = user

k210-serialport = /dev/ttyUSB0

kernelimg = $T/kernel.bin
k210 = $/k210.bin

DFLAGS = -D 

RUSTSBI = ./rustsbi/sbi-k210

QEMUOPTS = -machine virt -bios none -kernel $T/kernel -m 8M -smp $(CPUS) -nographic
QEMUOPTS += -bios $(RUSTSBI)
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

GDBPORT = 26000

ifeq ($(platform), k210)
RUSTSBI = ./rustsbi/sbi-k210
else
RUSTSBI = ./rustsbi/sbi-qemu
endif

$T/kernel:
	cd kern;$(MAKE) kernel

fs.img:
	cd user; $(MAKE) fs.img

sd  = /dev/sdb

sd : fs.img
	@if [ "$(sd)" != "" ]; then \
		echo "flashing into sd card..."; \
		sudo dd if=fs.img of=$(sd); \
	else \
		echo "sd card not detected!"; fi

qemu: $T/kernel fs.img 
	$(QEMU) $(QEMUOPTS)

all:
	cd kern; $(MAKE) kernel

k210: $T/kernel
	@$(OBJCOPY) $T/kernel --strip-all -O binary $(kernelImg)
	@$(OBJCOPY) $(RUSTSBI) --strip-all -O binary $(k210)
	@dd if=$(kernelImg) of=$(k210) bs=128k seek=1
	@$(OBJDUMP) -D -b binary -m riscv $(k210) > $T/k210.asm
	# @sudo chmod 777 $(k210-serialport)
	python3 ./tools/kflash.py -p $(k210-serialport) -b 1500000 -t $(k210)

clean:
	cd kern; $(MAKE) clean
	cd user; $(MAKE) clean
	rm -f $T/*.bin $T/*.asm	$T/*.sym $T/kernel k210.bin

cleansbi:
	cd ./rustsbi/rustsbi-k210; cargo clean
	cd ./rustsbi/rustsbi-qemu; cargo clean