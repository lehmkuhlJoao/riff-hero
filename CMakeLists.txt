cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

# Forçar UTF-8 mais agressivamente
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(UNICODE _UNICODE)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Para todos os compiladores
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8")

add_executable(main src/main.cpp)
target_compile_features(main PRIVATE cxx_std_20)
target_link_libraries(main PRIVATE SFML::Graphics SFML::System SFML::Window SFML::Network SFML::Audio -static-libgcc -static-libstdc++ -static)

set_target_properties(main PROPERTIES
        OUTPUT_NAME "Riff Hero"
)