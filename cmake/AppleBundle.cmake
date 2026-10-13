# macOS app bundle metadata (area F). Included from the top-level CMakeLists
# after the app target exists; does nothing on Linux or iOS (Qt generates the
# iOS plist).
if(APPLE AND NOT IOS AND TARGET hamclock-pro)
    set_target_properties(hamclock-pro PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/apple/Info.plist.in
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
endif()
