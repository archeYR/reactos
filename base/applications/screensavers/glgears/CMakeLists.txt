
list(APPEND SOURCE
    wglgears.c
    settings.c)

add_rc_deps(rsrc.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/icon_glgears.ico)
add_executable(glgears ${SOURCE} rsrc.rc)
set_module_type(glgears win32gui UNICODE)
set_target_properties(glgears PROPERTIES SUFFIX ".scr")
target_link_libraries(glgears scrnsave)
add_importlibs(glgears user32 gdi32 opengl32 glu32 winmm advapi32 msvcrt kernel32 ntdll)
add_pch(glgears glgears.h SOURCE)
add_cd_file(TARGET glgears DESTINATION reactos/system32 FOR all)
