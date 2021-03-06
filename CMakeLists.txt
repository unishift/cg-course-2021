cmake_minimum_required(VERSION 3.5)
project(SpaceObjects)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/common.h
        src/glad.c
        src/main.cpp
        src/ShaderProgram.h
        src/ShaderProgram.cpp
        src/Object.h
        src/Object.cpp
        src/Model.h
        src/Model.cpp
        src/ModelFactories.h
        src/ModelFactories.cpp
        src/BBox.h
        src/BBox.cpp
        src/Camera.h src/Font.cpp src/Font.h src/ShadowMap.cpp src/ShadowMap.h)

set(ADDITIONAL_INCLUDE_DIRS
        dependencies/include/GLAD)
set(ADDITIONAL_LIBRARY_DIRS
        dependencies/lib)
set(ADDITIONAL_RUNTIME_LIBRARY_DIRS
        dependencies/bin)

set (CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG}")

add_definitions("-DGLM_ENABLE_EXPERIMENTAL")

if(WIN32)
    set(ADDITIONAL_INCLUDE_DIRS
        ${ADDITIONAL_INCLUDE_DIRS}
        dependencies/include)
    link_directories(${ADDITIONAL_LIBRARY_DIRS})
else()
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
    find_package(glfw3 REQUIRED)
endif()

include_directories(${ADDITIONAL_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(DevIL REQUIRED)
find_package(Freetype REQUIRED)

add_executable(main ${SOURCE_FILES})

target_include_directories(main PRIVATE ${OPENGL_INCLUDE_DIR} ${IL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})
if(DEVELOP_MODE)
    add_custom_command(TARGET main POST_BUILD COMMAND ln -sfn "${PROJECT_SOURCE_DIR}/shaders" "${PROJECT_BINARY_DIR}/shaders")
    add_custom_command(TARGET main POST_BUILD COMMAND ln -sfn "${PROJECT_SOURCE_DIR}/models" "${PROJECT_BINARY_DIR}/models")
else()
    add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/shaders" "${PROJECT_BINARY_DIR}/shaders")
    add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/models" "${PROJECT_BINARY_DIR}/models")
endif()

if(WIN32)
    add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/dependencies/bin" $<TARGET_FILE_DIR:main>)
    #set(CMAKE_MSVCIDE_RUN_PATH ${ADDITIONAL_RUNTIME_LIBRARY_DIRS})
    target_compile_options(main PRIVATE)
    target_link_libraries(main LINK_PUBLIC ${OPENGL_gl_LIBRARY} glfw3dll glm assimp ${IL_LIBRARIES} ${ILU_LIBRARIES} ${FREETYPE_LIBRARIES})
else()
    target_compile_options(main PRIVATE -Wnarrowing)
    target_link_libraries(main LINK_PUBLIC ${OPENGL_gl_LIBRARY} glfw rt dl glm assimp ${IL_LIBRARIES} ${ILU_LIBRARIES} ${FREETYPE_LIBRARIES})
endif()

