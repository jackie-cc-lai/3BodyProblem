cmake_minimum_required(VERSION 3.16)
project(NBodyProblem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TESTING OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SFML)

add_executable(NBodyProblem
    main.cpp
    physics/calculate.cpp
    display/display.cpp
    display/grid.cpp
    display/body_editor.cpp
)

target_include_directories(NBodyProblem PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(NBodyProblem PRIVATE sfml-graphics sfml-window sfml-system)

if (MINGW)
    target_link_options(NBodyProblem PRIVATE "-Wl,--stack,8388608")
endif()

if (WIN32)
    set(DIST_DIR ${CMAKE_SOURCE_DIR}/dist)
    add_custom_command(TARGET NBodyProblem POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:NBodyProblem>
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            ${DIST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE_DIR:NBodyProblem>
    )
endif()
