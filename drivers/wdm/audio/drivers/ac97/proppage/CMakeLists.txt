
spec2def(ac97prop.dll ac97prop.spec)

add_library(ac97prop MODULE
    ac97prop.cpp
    AC97prop.rc)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ac97prop PRIVATE -Wno-unused-but-set-variable)
endif()

set_module_type(ac97prop win32dll UNICODE)
add_importlibs(ac97prop user32 comctl32 setupapi msvcrt kernel32)
add_cd_file(TARGET ac97prop DESTINATION reactos/system32 FOR all)
