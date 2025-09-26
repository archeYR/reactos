
list(APPEND HAL_SFI_ASM_SOURCE
    generic/v86.S)

list(APPEND HAL_SFI_SOURCE
    generic/beep.c
    generic/cmos.c
    generic/display.c
    generic/dma.c
    generic/drive.c
    generic/halinit.c
    generic/kdpci.c
    generic/memory.c
    generic/misc.c
    generic/nmi.c
    generic/sysinfo.c
    generic/usage.c
    generic/bios.c
    generic/portio.c
    generic/x86bios.c
    legacy/bus/bushndlr.c
    legacy/bus/cmosbus.c
    legacy/bus/isabus.c
    legacy/bus/pcibus.c
    ${CMAKE_CURRENT_BINARY_DIR}/pci_classes.c
    ${CMAKE_CURRENT_BINARY_DIR}/pci_vendors.c
    legacy/bus/sysbus.c
    legacy/bussupp.c
    legacy/halpnpdd.c
    legacy/halpcat.c
    sfi/apic.c
    sfi/pic.c
    sfi/reboot.c)

add_asm_files(lib_hal_sfi_asm ${HAL_SFI_ASM_SOURCE})
add_library(lib_hal_sfi OBJECT ${HAL_SFI_SOURCE} ${lib_hal_sfi_asm})
add_dependencies(lib_hal_sfi bugcodes xdk asm)
target_compile_definitions(lib_hal_sfi PRIVATE SARCH_SFI)
