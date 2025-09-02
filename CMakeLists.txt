cmake_minimum_required(VERSION 3.16)
project(OpenGothicStarter VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})
endif()

# Set compile flags
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Source files
set(SOURCES
    src/main.cpp
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

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
