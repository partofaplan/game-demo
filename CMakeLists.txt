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
# Enhanced SDL2 finding for Windows
if(WIN32)
    # Look for SDL2 in common Windows locations
    set(SDL2_ROOT_DIR "${CMAKE_SOURCE_DIR}/third_party/SDL2" CACHE PATH "SDL2 root directory")

    # Try to find SDL2 using multiple methods
    find_package(SDL2 QUIET)

    if(NOT SDL2_FOUND)
        # Fallback: Manual SDL2 configuration for Windows
        find_path(SDL2_INCLUDE_DIR SDL.h
            HINTS ${SDL2_ROOT_DIR}/include
            PATHS
                $ENV{SDL2DIR}/include
                C:/SDL2/include
                C:/vcpkg/installed/x64-windows/include
                C:/vcpkg/installed/x86-windows/include
        )

        find_library(SDL2_LIBRARY
            NAMES SDL2 SDL2main
            HINTS ${SDL2_ROOT_DIR}/lib
            PATHS
                $ENV{SDL2DIR}/lib
                C:/SDL2/lib/x64
                C:/SDL2/lib/x86
                C:/vcpkg/installed/x64-windows/lib
                C:/vcpkg/installed/x86-windows/lib
        )

        find_library(SDL2main_LIBRARY
            NAMES SDL2main
            HINTS ${SDL2_ROOT_DIR}/lib
            PATHS
                $ENV{SDL2DIR}/lib
                C:/SDL2/lib/x64
                C:/SDL2/lib/x86
                C:/vcpkg/installed/x64-windows/lib
                C:/vcpkg/installed/x86-windows/lib
        )

        if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
            # Create SDL2 targets if not found by find_package
            add_library(SDL2::SDL2 UNKNOWN IMPORTED)
            set_target_properties(SDL2::SDL2 PROPERTIES
                IMPORTED_LOCATION ${SDL2_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIR}
            )

            if(SDL2main_LIBRARY)
                add_library(SDL2::SDL2main UNKNOWN IMPORTED)
                set_target_properties(SDL2::SDL2main PROPERTIES
                    IMPORTED_LOCATION ${SDL2main_LIBRARY}
                    INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIR}
                )
            endif()
            set(SDL2_FOUND TRUE)
        endif()
    endif()
else()
    find_package(SDL2 REQUIRED)
endif()

if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found! Please install SDL2 or set SDL2_ROOT_DIR")
endif()

# Create executable target
add_executable(tank_duel src/main.cpp)

# Set target properties
set_target_properties(tank_duel PROPERTIES
    OUTPUT_NAME "TankDuel"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
    WIN32_EXECUTABLE TRUE
)

# Windows-specific optimizations
if(WIN32)
    # Set subsystem for Windows (avoids console window)
    set_target_properties(tank_duel PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )

    # Add Windows-specific compile definitions
    target_compile_definitions(tank_duel PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

    # Enable high DPI awareness on Windows (optional resource file)
    if(EXISTS ${CMAKE_SOURCE_DIR}/windows/tankduel.rc)
        target_sources(tank_duel PRIVATE ${CMAKE_SOURCE_DIR}/windows/tankduel.rc)
    endif()
endif()

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

    # Enhanced SDL2 DLL installation for Windows
    # Try multiple common SDL2 locations
    set(SDL2_DLL_PATHS
        "${SDL2_ROOT_DIR}/bin"
        "${SDL2_ROOT_DIR}/lib"
        "$ENV{SDL2DIR}/bin"
        "$ENV{SDL2DIR}/lib"
        "C:/SDL2/bin"
        "C:/SDL2/lib/x64"
        "C:/SDL2/lib/x86"
        "C:/vcpkg/installed/x64-windows/bin"
        "C:/vcpkg/installed/x86-windows/bin"
    )

    foreach(SDL2_PATH ${SDL2_DLL_PATHS})
        if(EXISTS "${SDL2_PATH}")
            file(GLOB SDL2_DLLS "${SDL2_PATH}/*.dll")
            if(SDL2_DLLS)
                install(FILES ${SDL2_DLLS}
                    DESTINATION ${CMAKE_INSTALL_BINDIR}
                    OPTIONAL
                )
                break()
            endif()
        endif()
    endforeach()

    # Install Visual C++ Redistributable if needed
    if(MSVC)
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        include(InstallRequiredSystemLibraries)
        if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
            install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                COMPONENT System
            )
        endif()
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
