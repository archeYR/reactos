
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    enum.c
    log.c
    pnp.c
    power.c
    serenum.c
    string.c)

list(APPEND PCH_SKIP_SOURCE
    pchsrc.c
    guid.c)

add_library(serenum MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    serenum.rc)

target_compile_definitions(serenum PUBLIC
    DEBUG_USE_KDPRINT
    _WIN32_WINNT=0x602
    NTDDI_VERSION=0x06020000) # NTDDI_WIN8

add_pch(serenum pch.h "${PCH_SKIP_SOURCE}")
set_module_type(serenum kernelmodedriver)
target_link_libraries(serenum ntoskrnl_vista)
add_importlibs(serenum ntoskrnl hal)
add_cd_file(TARGET serenum DESTINATION reactos/system32/drivers FOR all)
