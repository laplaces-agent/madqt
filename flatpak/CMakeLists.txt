configure_file(
    org.xeyes.madqt.yml.in
    org.xeyes.madqt.yml
    @ONLY
)

find_program(FLATPAK_BUILDER flatpak-builder)
if(FLATPAK_BUILDER)
    add_custom_target(flatpak
        COMMAND ${FLATPAK_BUILDER}
            --user --install --force-clean
            ${CMAKE_CURRENT_BINARY_DIR}/flatpak-build
            ${CMAKE_CURRENT_BINARY_DIR}/org.xeyes.madqt.yml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building and installing Flatpak (org.xeyes.madqt)"
        VERBATIM
    )
else()
    message(STATUS "flatpak-builder not found — 'flatpak' target unavailable")
endif()
