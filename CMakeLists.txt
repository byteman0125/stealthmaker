cmake_minimum_required(VERSION 3.16)
project(StealthMaker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Main executable
add_executable(StealthMaker WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/WindowUtils.cpp
    src/ConfigManager.cpp
    src/TrayIcon.cpp
    src/BorderOverlay.cpp
    src/Injector.cpp
    resource.rc
)

target_include_directories(StealthMaker PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR})
target_compile_definitions(StealthMaker PRIVATE UNICODE _UNICODE)
target_link_libraries(StealthMaker PRIVATE
    user32
    shell32
    advapi32
    gdi32
    psapi
    comctl32
)

# Use only our manifest from resource.rc - disable linker's default to avoid duplicate
target_link_options(StealthMaker PRIVATE /MANIFEST:NO)

# Inject DLL - used to protect other process windows
add_library(InjectDll SHARED src/InjectDll.cpp)
target_include_directories(InjectDll PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(InjectDll PRIVATE user32)
set_target_properties(InjectDll PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Copy DLL next to exe
add_custom_command(TARGET StealthMaker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:InjectDll> $<TARGET_FILE_DIR:StealthMaker>
)
