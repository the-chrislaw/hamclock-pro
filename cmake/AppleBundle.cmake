# macOS app bundle metadata (area F). Included from the top-level CMakeLists
# after the app target exists; does nothing on Linux. The macOS part is skipped on
# iOS, which has its own block at the end (Qt generates the iOS plist).
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

# iOS/iPadOS bundle metadata. Qt generates the iOS Info.plist from these target
# properties; without a bundle version Xcode refuses to install the app and App
# Store Connect rejects the upload. The build number must increase for every
# TestFlight upload, so CI passes -DHAMCLOCK_BUILD_NUMBER=<run number>.
if(IOS AND TARGET hamclock-pro)
    set(HAMCLOCK_BUILD_NUMBER "1" CACHE STRING "CFBundleVersion for Apple uploads")
    set(HAMCLOCK_IOS_BUNDLE_ID "com.chrislaw.hamclockpro" CACHE STRING "iOS bundle identifier")
    set_target_properties(hamclock-pro PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "${HAMCLOCK_IOS_BUNDLE_ID}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${HAMCLOCK_BUILD_NUMBER}"
        # Custom plist: all four iPad orientations and a launch screen, which App
        # Store validation requires. ID and versions come from the Xcode attributes.
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/apple/Info-ios.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${HAMCLOCK_IOS_BUNDLE_ID}"
        XCODE_ATTRIBUTE_MARKETING_VERSION "${PROJECT_VERSION}"
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${HAMCLOCK_BUILD_NUMBER}"
        # 1 = iPhone, 2 = iPad: one universal app.
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        # Without an install path CMake's Xcode project archives as a generic
        # archive with no ApplicationProperties, and -exportArchive then offers
        # no distribution methods ("expected one {} but found app-store").
        XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
        XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)")
endif()
