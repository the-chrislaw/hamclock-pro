# macOS app bundle metadata (area F). Included from the top-level CMakeLists
# after the app target exists; does nothing on Linux or iOS (Qt generates the
# iOS plist).
if(APPLE AND NOT IOS AND TARGET hamclock-pro)
    set_target_properties(hamclock-pro PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/apple/Info.plist.in
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
endif()

# App icon: AppIcon.icns is built from packaging/apple/AppIcon.png with the
# system sips/iconutil tools (always present on macOS) and copied into Resources.
if(APPLE AND NOT IOS AND TARGET hamclock-pro)
    set(_icon_png ${CMAKE_CURRENT_SOURCE_DIR}/packaging/apple/AppIcon.png)
    set(_iconset ${CMAKE_CURRENT_BINARY_DIR}/AppIcon.iconset)
    set(_icns ${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns)
    set(_icon_cmds COMMAND ${CMAKE_COMMAND} -E make_directory ${_iconset})
    foreach(_s 16 32 128 256 512)
        math(EXPR _d "${_s} * 2")
        list(APPEND _icon_cmds
            COMMAND sips -z ${_s} ${_s} ${_icon_png} --out ${_iconset}/icon_${_s}x${_s}.png
            COMMAND sips -z ${_d} ${_d} ${_icon_png} --out ${_iconset}/icon_${_s}x${_s}@2x.png)
    endforeach()
    add_custom_command(OUTPUT ${_icns}
        ${_icon_cmds}
        COMMAND iconutil -c icns ${_iconset} -o ${_icns}
        DEPENDS ${_icon_png}
        COMMENT "Building AppIcon.icns"
        VERBATIM)
    target_sources(hamclock-pro PRIVATE ${_icns})
    set_source_files_properties(${_icns} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()
