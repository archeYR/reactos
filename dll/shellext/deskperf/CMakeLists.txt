
spec2def(deskperf.dll deskperf.spec)

list(APPEND SOURCE
    deskperf.c
    shxiface.c
    precomp.h)

add_rc_deps(deskperf.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/monitor.ico)

add_library(deskperf MODULE
    ${SOURCE}
    deskperf.rc
    ${CMAKE_CURRENT_BINARY_DIR}/deskperf.def)

set_module_type(deskperf win32dll UNICODE)
target_link_libraries(deskperf uuid)
add_importlibs(deskperf advapi32 user32 gdi32 comctl32 ole32 msvcrt kernel32 ntdll)
add_pch(deskperf precomp.h SOURCE)
add_cd_file(TARGET deskperf DESTINATION reactos/system32 FOR all)
