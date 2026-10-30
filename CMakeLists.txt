cmake_minimum_required(VERSION 3.14)
project(SecureDocServer VERSION 1.0.0 LANGUAGES CXX)

# Require C++17 for std::filesystem and modern utilities
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find multi-threading support (required for background file watcher & server workers)
find_package(Threads REQUIRED)

# Automatically download and integrate IXWebSocket using FetchContent
include(FetchContent)
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v11.4.3
)
FetchContent_MakeAvailable(ixwebsocket)

# Define our server executable target
add_executable(SecureDocServer docserver.cpp)

target_compile_definitions(SecureDocServer PRIVATE
    SECUREDOC_DOCUMENT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/documents"
)

# Link IXWebSocket and threading libraries
target_link_libraries(SecureDocServer
    PRIVATE
        ixwebsocket
        Threads::Threads
)

# Platform-specific system libraries required by IXWebSocket
if(WIN32)
    target_link_libraries(SecureDocServer PRIVATE ws2_32 crypt32 secur32)
elseif(APPLE)
    target_link_libraries(SecureDocServer PRIVATE "-framework Security" "-framework CoreFoundation")
endif() 


