cmake_minimum_required(VERSION 3.16)

# Project Information
project(CodePlaceEditor 
    VERSION 0.2.0 
    LANGUAGES CXX
    DESCRIPTION "Modern, efficient code editor"
    HOMEPAGE_URL "https://github.com/co3ndev/codeplace-editor"
)

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

# AI sources
set(AI_SOURCES
    src/core/ai_client.h
    src/core/ai_client.cpp
    src/ai/ai_chat_sidebar.h
    src/ai/ai_chat_sidebar.cpp
    src/ai/ai_message_widget.h
    src/ai/ai_message_widget.cpp
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

# --- Executable & Linking ---

add_executable(codeplace
    ${CORE_SOURCES}
    ${EDITOR_SOURCES}
    ${BROWSER_SOURCES}
    ${TERMINAL_SOURCES}
    ${SEARCH_SOURCES}
    ${OUTLINE_SOURCES}
    ${GITSCM_SOURCES}
    ${LSPCLIENT_SOURCES}
    ${AI_SOURCES}
    ${MAIN_SOURCES}
)


target_compile_options(codeplace PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(codeplace PRIVATE 
    Qt6::Widgets 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Network 
    Qt6::Concurrent
    util
)

# --- Installation ---

include(GNUInstallDirs)

install(TARGETS codeplace
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Desktop file and Icon
install(FILES resources/codeplace.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES resources/codeplace.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps)

# Installation documentation and script for Tarball distribution
install(FILES resources/INSTALL.md resources/install.sh DESTINATION .)

# --- Distribution Packaging (CPack) ---

set(CPACK_PACKAGE_NAME "codeplace-editor")
set(CPACK_PACKAGE_VENDOR "CodePlace")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION "CodePlace Editor is a fast, lightweight, and modern code editor designed for productivity, built with C++ and Qt6.")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_CONTACT "Michael <developer@michaelcoen.dev>")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CodePlaceEditor")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Generators: DEB for Debian/Ubuntu, TGZ for Tarballs
set(CPACK_GENERATOR "DEB;TGZ")

# Debian Specifics
set(CPACK_DEBIAN_PACKAGE_NAME "codeplace-editor")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Michael <michael@codeplace.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# Additional runtime dependencies for plugins that are not automatically detected
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-qpa-plugins, qt6-wayland, libxkbcommon-x11-0, libqt6svg6")

set(CPACK_STRIP_FILES TRUE)

include(CPack)
