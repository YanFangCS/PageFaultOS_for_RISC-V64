OUTPUT_ARCH(riscv)
ENTRY(kernel_entry)

BASE_ADDRESS = 0xFFFFFFFFC0200000;

SECTIONS
{
    . = BASE_ADDRESS;

    .text : {
        *(.text.kernel_entry)
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }

    PROVIDE(etext = .);

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    . = ALIGN(0x1000);

    .data : {
        *(.data)
        *(.data.*)
    }

    .sdata : {
        *(.sdata)
        *(.sdata.*)
    }

    PROVIDE(edata = .);

    .bss = {
        *(.bss)
        *(.bss.*)
        *(.sbss*)
    }

    PROVIDE(end = .);
    
}