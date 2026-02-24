cmake_minimum_required(VERSION 3.16)

project(CodePlaceEditor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network Concurrent)

# Include directories
include_directories(src)

# Core sources
set(CORE_SOURCES
    src/core/default_shortcuts.h
    src/core/session_manager.h
    src/core/session_manager.cpp
    src/core/settings_manager.h
    src/core/settings_manager.cpp
    src/core/simple_highlighter.h
    src/core/simple_highlighter.cpp
    src/core/theme_manager.h
    src/core/theme_manager.cpp
    src/core/process_helper.h
    src/core/process_helper.cpp
    src/core/project_manager.h
    src/core/project_manager.cpp
)

# Editor sources
set(EDITOR_SOURCES
    src/editor/editor_view.h
    src/editor/editor_view.cpp
    src/editor/find_replace_bar.h
    src/editor/find_replace_bar.cpp
    src/editor/tab_container.h
    src/editor/tab_container.cpp
    src/editor/quick_start_widget.h
    src/editor/quick_start_widget.cpp
    src/editor/symbol_info_widget.h
    src/editor/symbol_info_widget.cpp
)

# Browser sources
set(BROWSER_SOURCES
    src/browser/file_browser_sidebar.h
    src/browser/file_browser_sidebar.cpp
)

# Terminal sources
set(TERMINAL_SOURCES
    src/terminal/terminal_widget.h
    src/terminal/terminal_widget.cpp
    src/terminal/pty_process.h
    src/terminal/pty_process.cpp
)
 
 # Search sources
 set(SEARCH_SOURCES
     src/search/search_engine.h
     src/search/search_engine.cpp
     src/search/search_widget.h
     src/search/search_widget.cpp
 )

# Outline sources
set(OUTLINE_SOURCES
    src/outline/outline_widget.h
    src/outline/outline_widget.cpp
    src/outline/symbol_parser.h
    src/outline/symbol_parser.cpp
)

set(GITSCM_SOURCES
    src/gitscm/git_manager.h
    src/gitscm/git_manager.cpp
    src/gitscm/gitscm_widget.h
    src/gitscm/gitscm_widget.cpp
    src/gitscm/diff_view.h
    src/gitscm/diff_view.cpp
    src/gitscm/diff_utils.h
    src/gitscm/diff_utils.cpp
    src/gitscm/create_branch_dialog.h
    src/gitscm/create_branch_dialog.cpp
)

# LSP Client sources
set(LSPCLIENT_SOURCES
    src/lspclient/lspclient_widget.h
    src/lspclient/lspclient_widget.cpp
    src/lspclient/lsp_process.h
    src/lspclient/lsp_process.cpp
    src/lspclient/lsp_client.h
    src/lspclient/lsp_client.cpp
    src/lspclient/lsp_manager.h
    src/lspclient/lsp_manager.cpp
    src/lspclient/problems_widget.h
    src/lspclient/problems_widget.cpp
)

# Main sources
set(MAIN_SOURCES
    src/main.cpp
    src/main/main_window.h
    src/main/main_window.cpp
    src/main/preferences_dialog.h
    src/main/preferences_dialog.cpp
    resources.qrc
)

add_executable(codeplace
    ${CORE_SOURCES}
    ${EDITOR_SOURCES}
    ${BROWSER_SOURCES}
    ${TERMINAL_SOURCES}
    ${SEARCH_SOURCES}
    ${OUTLINE_SOURCES}
    ${GITSCM_SOURCES}
    ${LSPCLIENT_SOURCES}
    ${MAIN_SOURCES}
)

target_compile_options(codeplace PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(codeplace PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network Qt6::Concurrent)

if(UNIX)
    target_link_libraries(codeplace PRIVATE util)
endif()

# Deployment/Packaging
if(TARGET codeplace)
    include(GNUInstallDirs)
    
    # Core Binary
    install(TARGETS codeplace
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Resources (Icons, Desktop Entry)
    if(UNIX AND NOT APPLE)
        install(FILES resources/codeplace.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
        install(FILES resources/codeplace.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps)
    endif()

    # Qt Deployment
    if(Qt6_FOUND)
        if(COMMAND qt_generate_deploy_app_script)
            qt_generate_deploy_app_script(
                TARGET codeplace
                FILENAME_VARIABLE deploy_script
                NO_UNSUPPORTED_PLATFORM_ERROR
            )
            install(SCRIPT ${deploy_script})
        endif()
    endif()

    # CPack Configuration
    set(CPACK_PACKAGE_NAME "codeplace-editor")
    set(CPACK_PACKAGE_VENDOR "CodePlace")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++ Code Editor")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/codeplace/editor")
    set(CPACK_PACKAGE_CONTACT "Michael <developer@michaelcoen.dev>")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "CodePlaceEditor")
    
    if(APPLE)
        set(CPACK_GENERATOR "DragNDrop")
    else()
        set(CPACK_GENERATOR "TGZ;DEB")
        set(CPACK_DEBIAN_PACKAGE_NAME "codeplace-editor")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Michael <michael@codeplace.com>")
        set(CPACK_PACKAGE_DESCRIPTION "CodePlace Editor is a fast, lightweight, and modern C++ code editor designed for productivity.")
        set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
        set(CPACK_STRIP_FILES TRUE)
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    endif()

    include(CPack)
endif()
