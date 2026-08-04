vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO austinkottke/Qivot
    REF v1.0.0
    SHA512 "please-update-with-actual-sha512-hash"
    HEAD_REF main
)

vcpkg_check_features(
    OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        network WITH_NETWORK
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DQIVOT_WITH_NETWORK=${VCPKG_FEATURE_NETWORK}
        -DQIVOT_BUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/Qivot)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
