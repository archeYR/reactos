
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(mountvol mountvol.c mountvol.rc)
set_module_type(mountvol win32cui UNICODE)
target_link_libraries(mountvol conutils ${PSEH_LIB})
add_importlibs(mountvol msvcrt kernel32 ntdll)
add_cd_file(TARGET mountvol DESTINATION reactos/system32 FOR all)
