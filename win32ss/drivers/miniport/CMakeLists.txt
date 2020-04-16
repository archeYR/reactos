
add_subdirectory(vga)
add_subdirectory(vmx_svga)

if(ARCH STREQUAL "i386")
    add_subdirectory(xboxvmp)
endif()
