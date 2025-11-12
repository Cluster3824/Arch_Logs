cmake_minimum_required(VERSION 3.10)
project(archlog VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Main executable - simplified single file build
add_executable(archlog src/main.cpp)

# Installation rules
include(GNUInstallDirs)

install(TARGETS archlog
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/man/archlog.1")
    install(FILES man/archlog.1
            DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/completion/archlog-completion.bash")
    install(FILES completion/archlog-completion.bash
            DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
            RENAME archlog)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    install(FILES LICENSE
            DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/archlog)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install(FILES README.md
            DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()