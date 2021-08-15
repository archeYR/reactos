
add_message_headers(ANSI serlog.mc)

remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    error.c
    flush.c
    immediat.c
    initunlo.c
    ioctl.c
    isr.c
    log.c
    modmflow.c
    openclos.c
    pnp.c
    power.c
    purge.c
    qsfile.c
    read.c
    registry.c
    utils.c
    waitmask.c
    #wmi.c
    write.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    precompsrc.c)

add_library(serial MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    serial.rc)

set_module_type(serial kernelmodedriver)
target_compile_definitions(serial PRIVATE DEBUG_USE_KDPRINT)
target_link_libraries(serial rtlver wdf01000 ntoskrnl_vista ${PSEH_LIB})
add_importlibs(serial ntoskrnl hal)
add_dependencies(serial serlog)
# add_pch(serial serial.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET serial DESTINATION reactos/system32/drivers FOR all)
