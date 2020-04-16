
list(APPEND SOURCE
    enable.c
    palette.c
    pointer.c
    screen.c
    surface.c
    framebuf.h)

add_library(framebuf_old MODULE
    ${SOURCE}
    framebuf.rc)

set_module_type(framebuf_old kerneldll ENTRYPOINT DrvEnableDriver 12)
add_pch(framebuf_old framebuf.h SOURCE)
target_link_libraries(framebuf_old libcntpr)
add_importlibs(framebuf_old win32k)
add_dependencies(framebuf_old psdk)
add_cd_file(TARGET framebuf_old DESTINATION reactos/system32 FOR all)
