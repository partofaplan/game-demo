# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(TankDuel
    VERSION 1.0.0
    DESCRIPTION "Tank Duel Game"
    LANGUAGES CXX
)

# Modern CMake standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find dependencies
find_package(SDL2 REQUIRED)

# Create executable target
add_executable(tank_duel src/main.cpp)

# Set target properties
set_target_properties(tank_duel PROPERTIES
    OUTPUT_NAME "TankDuel"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
    WIN32_EXECUTABLE TRUE
)

# Link libraries
target_link_libraries(tank_duel PRIVATE SDL2::SDL2 SDL2::SDL2main)

# Include GNUInstallDirs for standard installation directories
include(GNUInstallDirs)

# Installation rules
if(APPLE)
    # macOS App Bundle installation
    install(TARGETS tank_duel
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install SDL2 frameworks if needed
    if(SDL2_LIBRARIES)
        install(FILES ${SDL2_LIBRARIES}
            DESTINATION TankDuel.app/Contents/Frameworks
            OPTIONAL
        )
    endif()
elseif(WIN32)
    # Windows installation
    install(TARGETS tank_duel
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install SDL2 DLLs
    if(EXISTS "${SDL2_BINDIR}")
        file(GLOB SDL2_DLLS "${SDL2_BINDIR}/*.dll")
        install(FILES ${SDL2_DLLS}
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            OPTIONAL
        )
    endif()
else()
    # Linux and other Unix-like systems
    install(TARGETS tank_duel
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install desktop entry
    install(FILES ${CMAKE_SOURCE_DIR}/tankduel.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        OPTIONAL
    )

    # Install icon
    install(FILES ${CMAKE_SOURCE_DIR}/tankduel.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps
        OPTIONAL
    )
endif()

# Install documentation
install(FILES README.md LICENSE
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    OPTIONAL
)

# CPack configuration for packaging
include(CPack)

# Basic CPack settings
set(CPACK_PACKAGE_NAME "TankDuel")
set(CPACK_PACKAGE_VENDOR "TankDuel Development Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tank Duel - An exciting tank battle game")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "TankDuel")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Platform-specific packaging
if(APPLE)
    # macOS packaging
    set(CPACK_GENERATOR "DragNDrop;ZIP")
    set(CPACK_DMG_VOLUME_NAME "TankDuel")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_BUNDLE_NAME "TankDuel")
    set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/Info.plist.in")
    set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/tankduel.icns")
elseif(WIN32)
    # Windows packaging
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "TankDuel")
    set(CPACK_NSIS_PACKAGE_NAME "TankDuel")
    set(CPACK_NSIS_CONTACT "support@tankduel.com")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/yourname/tank-duel")
    set(CPACK_NSIS_HELP_LINK "https://github.com/yourname/tank-duel/wiki")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
else()
    # Linux packaging
    set(CPACK_GENERATOR "DEB;RPM;TGZ")

    # DEB package settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "TankDuel Development Team")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Tank Duel Game
 An exciting tank battle game built with SDL2.
 Features include multiplayer battles, various tank types,
 and dynamic battlefields.")

    # RPM package settings
    set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_REQUIRES "SDL2")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "Tank Duel Game
An exciting tank battle game built with SDL2.
Features include multiplayer battles, various tank types,
and dynamic battlefields.")
endif()

# Source package
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\.git/"
    "/build/"
    "/\\.DS_Store"
    "/Thumbs\\.db"
    "\\.gitignore"
    "\\.gitattributes"
)
