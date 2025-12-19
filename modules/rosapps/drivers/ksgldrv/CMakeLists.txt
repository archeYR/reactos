
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    device.c
    device.h
    driver.c
    driver.h
    public.h
    queue.c
    queue.h)

add_library(ksgldrv MODULE ${SOURCE})
set_module_type(ksgldrv kmdfdriver)

if(NOT MSVC)
    target_compile_options(ksgldrv PRIVATE
        -Wno-discarded-qualifiers
        -Wno-incompatible-pointer-types)
endif()

target_link_libraries(ksgldrv libcntpr ${PSEH_LIB})
add_importlibs(ksgldrv wdfldr ntoskrnl hal)
add_cd_file(TARGET ksgldrv DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(ksgldrv ksgldrv.inf)
