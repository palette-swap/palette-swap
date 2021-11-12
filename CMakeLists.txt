cmake_minimum_required(VERSION 3.1)
project(palette-swap)

# Set c++11
# https://stackoverflow.com/questions/10851247/how-to-activate-c-11-in-cmake
if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif ()
set (CMAKE_CXX_STANDARD 17)

# nice hierarchichal structure in MSVC
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# https://stackoverflow.com/questions/40779571/adding-crt-secure-no-warnings-definition-using-cmake
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

#Find OS
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(IS_OS_MAC 1)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(IS_OS_LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(IS_OS_WINDOWS 1)
else()
  message(FATAL_ERROR "OS ${CMAKE_SYSTEM_NAME} was not recognized")
endif()

# Create executable target

# Generate the shader folder location to the header
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ext/project_path.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/ext/project_path.hpp")

# You can switch to use the file GLOB for simplicity but at your own risk
file(GLOB SOURCE_FILES src/*.cpp src/*.hpp)

# external libraries will be installed into /usr/local/include and /usr/local/lib but that folder is not automatically included in the search on MACs
if (IS_OS_MAC)
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC src/)

# Added this so policy CMP0065 doesn't scream
set_target_properties(${PROJECT_NAME} PROPERTIES ENABLE_EXPORTS 0)

# External header-only libraries in the ext/
target_include_directories(${PROJECT_NAME} PUBLIC ext/stb_image/)
target_include_directories(${PROJECT_NAME} PUBLIC ext/gl3w)
target_include_directories(${PROJECT_NAME} PUBLIC ext/entt)
target_include_directories(${PROJECT_NAME} PUBLIC ext)


# Disable VS Error checking on ext folder
# Idea sourced from https://stackoverflow.com/questions/1005901/how-to-set-path-environment-variable-using-cmake-and-visual-studio-to-run-test
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.user.props" [=[
<?xml version="1.0" encoding="utf-8"?> 
<Project ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <CAExcludePath>"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools";$(ProjectDir)ext;$(CAExcludePath)</CAExcludePath>
  </PropertyGroup>
</Project>
]=])

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        VS_USER_PROPS "${PROJECT_NAME}.user.props"
) 

# Find OpenGL
find_package(OpenGL REQUIRED)

if (OPENGL_FOUND)
   target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
   target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENGL_gl_LIBRARY})
endif()

set(glm_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/glm/cmake/glm) # if necessary
find_package(glm REQUIRED)

# glfw, sdl could be precompiled (on windows) or installed by a package manager (on OSX and Linux)
if (IS_OS_LINUX OR IS_OS_MAC)
    # Try to find packages rather than to use the precompiled ones
    # Since we're on OSX or Linux, we can just use pkgconfig.
    find_package(PkgConfig REQUIRED)

    pkg_search_module(GLFW REQUIRED glfw3)

    pkg_search_module(SDL2 REQUIRED sdl2)
    pkg_search_module(SDL2_TTF REQUIRED sdl2_ttf)
    pkg_search_module(FREETYPE freetype2)
    pkg_search_module(ZLIB zlib)

    # Link Frameworks on OSX
    if (IS_OS_MAC)
       find_library(COCOA_LIBRARY Cocoa)
       find_library(CF_LIBRARY CoreFoundation)
       target_link_libraries(${PROJECT_NAME} PUBLIC ${COCOA_LIBRARY} ${CF_LIBRARY})
    endif()

    # Increase warning level
    target_compile_options(${PROJECT_NAME} PUBLIC "-Wall")
elseif (IS_OS_WINDOWS)
# https://stackoverflow.com/questions/17126860/cmake-link-precompiled-library-depending-on-os-and-architecture
    set(GLFW_FOUND TRUE)
    set(SDL2_FOUND TRUE)

    set(GLFW_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/include")
    set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/include/SDL")

    if (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
        set(GLFW_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib/glfw3dll-x64.lib")
        set(SDL2_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/SDL2.lib")
        set(SDL2_TTF_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/SDL2_ttf.lib")

	    set(GLFW_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib/glfw3-x64.dll")
        set(SDL_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/SDL2.dll")
        set(SDL2_TTF_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/SDL2_ttf.dll")
        set(FREETYPE_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/libfreetype-6.dll")
        set(ZLIB1_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x64/zlib1.dll")
    else()
        set(GLFW_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib/glfw3dll-x86.lib")
        set(SDL2_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x86/SDL2.lib")
        set(SDL2_TTF_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x86/SDL2_ttf.lib")

	    set(GLFW_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib/glfw3-x86.dll")
        set(SDL_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x86/SDL2.dll")
        set(SDL2_TTF_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x86/SDL2_ttf.dll")
        set(FREETYPE_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x86/libfreetype-6.dll")
        set(ZLIB1_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/x86/zlib1.dll")
    endif()

    # Copy and rename dlls
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLFW_DLL}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/glfw3.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL_DLL}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_TTF_DLL}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2_ttf.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FREETYPE_DLL}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/libfreetype-6.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ZLIB1_DLL}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/zlib1.dll")

    target_compile_options(${PROJECT_NAME} PUBLIC
        # Turn warning "not all control paths return a value" into an error
        "/we4715"

        # use sane exception handling, rather than trying to catch segfaults and allowing resource
        # leaks and UB. Yup... See "Default exception handling behavior" at
        # https://docs.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model?view=vs-2019
        "/EHsc"

        # turn warning C4239 (non-standard extension that allows temporaries to be bound to
        # non-const references, yay microsoft) into an error
        "/we4239"
    )
endif()

# Can't find the include and lib. Quit.
if (NOT GLFW_FOUND OR NOT SDL2_FOUND)
   if (NOT GLFW_FOUND)
      message(FATAL_ERROR "Can't find GLFW." )
   else ()
      message(FATAL_ERROR "Can't find SDL." )
   endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${GLFW_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ext/soloud/include")

set(SDL2_LIBRARY_TEMP ${SDL2_LIBRARIES})
set(SDL2_INCLUDE_DIR ${SDL2_INCLUDE_DIRS})
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/soloud/contrib")
set_property(TARGET soloud PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/ext/soloud/bin/soloud.lib")

target_link_libraries(${PROJECT_NAME} PUBLIC ${GLFW_LIBRARIES} ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} soloud glm::glm)

# Needed to add this
if(IS_OS_LINUX)
  target_link_libraries(${PROJECT_NAME} PUBLIC glfw ${CMAKE_DL_LIBS})
endif()
