# Qivot builds a static library only: its classes carry no export macros.
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO austinkottke/Qivot
    REF "v${VERSION}"
    SHA512 e5aa27a7dd16a4adce77d96f04666eedc4f3791779d6e5d134db63a42cca1ae4237d520f71652f1a78c401cd6607c01b7601a2cb47caea1e91ff75764cd5f59a
    HEAD_REF main
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        network QIVOT_WITH_NETWORK
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DQIVOT_BUILD_EXAMPLES=OFF
        -DQIVOT_WITH_QML=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/Qivot)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
