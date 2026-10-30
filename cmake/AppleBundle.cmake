# macOS app bundle metadata (area F). Included from the top-level CMakeLists
# after the app target exists; does nothing on Linux. The macOS part is skipped on
# iOS, which has its own block at the end (Qt generates the iOS plist).
if(APPLE AND NOT IOS AND TARGET hamclock-pro)
    # The Mac App Store build shares the iOS App Store record (universal purchase),
    # so CI passes -DHAMCLOCK_MACOS_BUNDLE_ID=com.chrislaw.hamclockpro and a rising
    # -DHAMCLOCK_BUILD_NUMBER. The Developer ID dmg keeps the defaults.
    set(HAMCLOCK_MACOS_BUNDLE_ID "org.hamclockpro.app" CACHE STRING "macOS bundle identifier")
    set(_mac_build_number "${PROJECT_VERSION}")
    if(HAMCLOCK_BUILD_NUMBER)
        set(_mac_build_number "${HAMCLOCK_BUILD_NUMBER}")
    endif()
    set_target_properties(hamclock-pro PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "${HAMCLOCK_MACOS_BUNDLE_ID}"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/apple/Info.plist.in
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${_mac_build_number}")
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
    # Sky finder permission strings go in only when the finder is built in.
    set(HAMCLOCK_SKY_FINDER_PLIST_KEYS "")
    if(HAMCLOCK_SKY_FINDER)
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/packaging/apple/sky-finder-keys.plist.in" HAMCLOCK_SKY_FINDER_PLIST_KEYS)
    endif()
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
        XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
        # App icon from the asset catalog (opaque 1024 px; actool derives the
        # 120/152/167/180 px sizes and CFBundleIconName App Store validation wants).
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
    set(_ios_assets "${CMAKE_CURRENT_SOURCE_DIR}/packaging/apple/ios/Assets.xcassets")
    target_sources(hamclock-pro PRIVATE "${_ios_assets}")
    set_source_files_properties("${_ios_assets}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

# Privacy manifest (App Store requirement): no tracking, no collected data, and
# the required-reason API declarations. Bundled for macOS and iOS/iPadOS.
if(APPLE AND TARGET hamclock-pro)
    set(_privacy "${CMAKE_CURRENT_SOURCE_DIR}/packaging/apple/PrivacyInfo.xcprivacy")
    target_sources(hamclock-pro PRIVATE "${_privacy}")
    set_source_files_properties("${_privacy}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

# Qt for iOS is a static build: QML plugins (QtQuick, Controls, Layouts, ...)
# are only linked in when imported explicitly. Without this the app starts,
# fails with 'module "QtQuick" plugin "qtquick2plugin" not found' and exits.
# The QML files are added as plain resources (not a QML module), so point the
# import scanner at the source tree.
if(IOS AND TARGET hamclock-pro)
    qt_import_qml_plugins(hamclock-pro PATH_TO_SCAN "${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()
