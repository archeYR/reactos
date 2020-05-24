
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(comctrl comctrl.c)
set_module_type(comctrl win32cui UNICODE)
target_link_libraries(comctrl conutils ${PSEH_LIB})
add_importlibs(comctrl advapi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET comctrl DESTINATION reactos/system32 FOR all)
