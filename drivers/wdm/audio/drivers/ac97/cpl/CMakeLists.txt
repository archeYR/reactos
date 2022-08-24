
spec2def(ac97cpl.cpl ac97cpl.spec)
add_rc_deps(ac97cpl.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/ac97cpl.ico)

add_library(ac97cpl MODULE
    ac97cpl.cpp
    ac97cpl.rc)

set_module_type(ac97cpl cpl UNICODE)
add_importlibs(ac97cpl comctl32 shell32 setupapi user32 msvcrt kernel32)
add_cd_file(TARGET ac97cpl DESTINATION reactos/system32 FOR all)
