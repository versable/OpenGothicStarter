cmake_minimum_required(VERSION 3.16)
project(OpenGothicStarter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(OGS_STRICT_WARNINGS "Enable strict compiler warnings." ON)
option(OGS_WARNINGS_AS_ERRORS "Treat warnings as errors." OFF)
option(OGS_EXTRA_WARNINGS "Enable additional warning checks." OFF)
option(OGS_HARDENED_BUILD "Enable compiler/linker hardening flags." OFF)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})
endif()

# Optional: pe-parse for extracting icons from Windows executables on Unix-like
# platforms.
set(OGS_BUNDLED_PEPARSE OFF)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/pe-parse/CMakeLists.txt")
    # Keep the vendored dependency lightweight in this project.
    set(BUILD_COMMAND_LINE_TOOLS OFF CACHE BOOL "" FORCE)
    set(PEPARSE_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(PEPARSE_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    add_subdirectory(lib/pe-parse EXCLUDE_FROM_ALL)
    if(TARGET pe-parse::pe-parse)
        set(OGS_BUNDLED_PEPARSE ON)
    endif()
endif()

if(NOT OGS_BUNDLED_PEPARSE)
    find_path(PEPARSE_INCLUDE_DIR pe-parse/parse.h)
    find_library(PEPARSE_LIBRARY NAMES pe-parse peparse pe-parser-library)
endif()

if(OGS_BUNDLED_PEPARSE OR (PEPARSE_INCLUDE_DIR AND PEPARSE_LIBRARY))
    message(STATUS "pe-parse found: enabling PE icon extraction support")
else()
    message(STATUS "pe-parse not found: PE icon extraction support disabled")
endif()

# Source files
set(SOURCES
    src/app.cpp
    src/pe_icon_loader.cpp
    src/runtime_paths.cpp
)

# Create executable - Use WIN32 flag for Windows GUI apps
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# Windows-specific settings
if(WIN32)
    # Ensure GUI subsystem is used
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # Add wxWidgets definitions for Windows
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        wxUSE_GUI=1
        __WXMSW__
    )
endif()

if(OGS_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /w14242
            /w14254
            /w14263
            /w14265
            /w14287
            /w14296
        )
        if(OGS_EXTRA_WARNINGS)
            target_compile_options(${PROJECT_NAME} PRIVATE
                /w44061
                /w44062
            )
        endif()
        if(OGS_WARNINGS_AS_ERRORS)
            target_compile_options(${PROJECT_NAME} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wformat=2
            -Wundef
            -Wnon-virtual-dtor
        )
        if(OGS_EXTRA_WARNINGS)
            target_compile_options(${PROJECT_NAME} PRIVATE
                -Wold-style-cast
                -Wconversion
                -Wsign-conversion
                -Wdouble-promotion
            )
        endif()
        if(OGS_WARNINGS_AS_ERRORS)
            target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
        endif()
    endif()
endif()

if(OGS_HARDENED_BUILD AND NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -fstack-protector-strong
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:_FORTIFY_SOURCE=3>
    )

    if(NOT APPLE)
        target_compile_options(${PROJECT_NAME} PRIVATE
            -fPIE
            -fstack-clash-protection
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            -pie
            -Wl,-z,relro
            -Wl,-z,now
            -Wl,-z,noexecstack
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -ftrivial-auto-var-init=zero
        )
    endif()
endif()

# ASan (Note: fixed typo in comment)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT WIN32)  # ASan typically not used with MSVC
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    endif()
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} ${wxWidgets_LIBRARIES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${wxWidgets_INCLUDE_DIRS})

if(OGS_BUNDLED_PEPARSE)
    target_link_libraries(${PROJECT_NAME} pe-parse::pe-parse)
    target_compile_definitions(${PROJECT_NAME} PRIVATE OGS_HAVE_PE_PARSE=1)
elseif(PEPARSE_INCLUDE_DIR AND PEPARSE_LIBRARY)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PEPARSE_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${PEPARSE_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PRIVATE OGS_HAVE_PE_PARSE=1)
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
