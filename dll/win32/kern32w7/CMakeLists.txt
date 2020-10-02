
spec2def(kern32w7.dll kern32w7.spec ADD_IMPORTLIB)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D_KERNEL32_)

list(APPEND SOURCE
    DllMain.c
)

add_library(kern32w7 MODULE ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/kern32w7.def)
set_module_type(kern32w7 win32dll ENTRYPOINT DllMain 12)
add_importlibs(kern32w7 kernel32 ntdll_vista ntdll)
add_delay_importlibs(kern32w7 ntdll_vista)
add_dependencies(kern32w7 psdk)
add_cd_file(TARGET kern32w7 DESTINATION reactos/system32 FOR all)
