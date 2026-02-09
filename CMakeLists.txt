cmake_minimum_required(VERSION 3.16)
project(OpenGothicStarter VERSION 0.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(OGS_STRICT_WARNINGS "Enable strict compiler warnings." ON)
option(OGS_WARNINGS_AS_ERRORS "Treat warnings as errors." OFF)
option(OGS_EXTRA_WARNINGS "Enable additional warning checks." OFF)
option(OGS_HARDENED_BUILD "Enable compiler/linker hardening flags." OFF)

set(OGS_I18N_DOMAIN "opengothicstarter")
set(OGS_I18N_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/i18n")
set(OGS_I18N_LOCALE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/locale")
set(OGS_I18N_INSTALL_DIR "share/locale")

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
    src/embedded_locales.cpp
    src/localization.cpp
    src/pe_icon_loader.cpp
    src/runtime_paths.cpp
    src/settings_dialog.cpp
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

# Optional gettext catalogs (.po -> .mo)
find_program(MSGFMT_EXECUTABLE msgfmt)
file(GLOB OGS_I18N_PO_FILES RELATIVE "${OGS_I18N_SOURCE_DIR}"
    "${OGS_I18N_SOURCE_DIR}/*/LC_MESSAGES/${OGS_I18N_DOMAIN}.po"
)

set(OGS_I18N_MO_FILES "")
set(OGS_I18N_LOCALES "")
set(OGS_EMBEDDED_LOCALE_DIR "${CMAKE_CURRENT_BINARY_DIR}/embedded")
set(OGS_EMBEDDED_LOCALE_ZIP "${OGS_EMBEDDED_LOCALE_DIR}/opengothicstarter-locales.zip")
set(OGS_EMBEDDED_LOCALE_SOURCE "${OGS_EMBEDDED_LOCALE_DIR}/embedded_locales_data.cpp")
set(OGS_EMBEDDED_LOCALE_DEPENDS "")

if(OGS_I18N_PO_FILES)
    if(MSGFMT_EXECUTABLE)
        foreach(PO_REL_PATH IN LISTS OGS_I18N_PO_FILES)
            string(REGEX REPLACE "/LC_MESSAGES/.*$" "" LOCALE_NAME "${PO_REL_PATH}")

            set(PO_FILE "${OGS_I18N_SOURCE_DIR}/${PO_REL_PATH}")
            set(MO_FILE "${OGS_I18N_LOCALE_OUTPUT_DIR}/${LOCALE_NAME}/LC_MESSAGES/${OGS_I18N_DOMAIN}.mo")

            add_custom_command(
                OUTPUT "${MO_FILE}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${OGS_I18N_LOCALE_OUTPUT_DIR}/${LOCALE_NAME}/LC_MESSAGES"
                COMMAND "${MSGFMT_EXECUTABLE}" -o "${MO_FILE}" "${PO_FILE}"
                DEPENDS "${PO_FILE}"
                VERBATIM
            )

            list(APPEND OGS_I18N_MO_FILES "${MO_FILE}")
            list(APPEND OGS_I18N_LOCALES "${LOCALE_NAME}")
        endforeach()

        add_custom_target(ogs_i18n_catalogs DEPENDS ${OGS_I18N_MO_FILES})
        add_dependencies(${PROJECT_NAME} ogs_i18n_catalogs)

        list(REMOVE_DUPLICATES OGS_I18N_LOCALES)
        list(SORT OGS_I18N_LOCALES)

        if(OGS_I18N_LOCALES)
            add_custom_command(
                OUTPUT "${OGS_EMBEDDED_LOCALE_ZIP}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${OGS_EMBEDDED_LOCALE_DIR}"
                COMMAND ${CMAKE_COMMAND} -E tar cf "${OGS_EMBEDDED_LOCALE_ZIP}" --format=zip ${OGS_I18N_LOCALES}
                WORKING_DIRECTORY "${OGS_I18N_LOCALE_OUTPUT_DIR}"
                DEPENDS ${OGS_I18N_MO_FILES}
                VERBATIM
            )
            list(APPEND OGS_EMBEDDED_LOCALE_DEPENDS "${OGS_EMBEDDED_LOCALE_ZIP}")
        endif()

        foreach(LOCALE_NAME IN LISTS OGS_I18N_LOCALES)
            set(MO_FILE "${OGS_I18N_LOCALE_OUTPUT_DIR}/${LOCALE_NAME}/LC_MESSAGES/${OGS_I18N_DOMAIN}.mo")

            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/locale/${LOCALE_NAME}/LC_MESSAGES"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MO_FILE}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/locale/${LOCALE_NAME}/LC_MESSAGES/${OGS_I18N_DOMAIN}.mo"
                VERBATIM
            )

            install(FILES "${MO_FILE}" OPTIONAL
                DESTINATION "${OGS_I18N_INSTALL_DIR}/${LOCALE_NAME}/LC_MESSAGES"
            )
        endforeach()
    else()
        message(FATAL_ERROR
            "Found i18n .po files but 'msgfmt' is not available. "
            "Install gettext tools or pass -DMSGFMT_EXECUTABLE=/path/to/msgfmt.")
    endif()
endif()

add_custom_command(
    OUTPUT "${OGS_EMBEDDED_LOCALE_SOURCE}"
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${OGS_EMBEDDED_LOCALE_ZIP}
        -DOUTPUT_FILE=${OGS_EMBEDDED_LOCALE_SOURCE}
        -DSYMBOL_NAME=gEmbeddedLocaleZip
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary_to_cpp.cmake
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary_to_cpp.cmake"
        ${OGS_EMBEDDED_LOCALE_DEPENDS}
    VERBATIM
)
target_sources(${PROJECT_NAME} PRIVATE "${OGS_EMBEDDED_LOCALE_SOURCE}")

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
