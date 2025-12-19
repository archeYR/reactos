
list(APPEND SOURCE
    platform/windrv.c
    glimpl.c
    pb.c
    scratch.c
    spinlock.c
    winmain.c)

add_library(sharedgl MODULE
    ${SOURCE})

if(NOT MSVC)
    target_compile_options(sharedgl PRIVATE
        -Wno-discarded-qualifiers
        -Wno-pointer-to-int-cast
        -Wno-int-to-pointer-cast
        -Wno-int-conversion
        -Wno-incompatible-pointer-types
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-but-set-variable)
endif()

set_module_type(sharedgl win32dll)
add_importlibs(sharedgl msvcrt ntdll kernel32 gdi32 opengl32 ws2_32 winmm setupapi)
add_cd_file(TARGET sharedgl DESTINATION reactos/system32 FOR all)
