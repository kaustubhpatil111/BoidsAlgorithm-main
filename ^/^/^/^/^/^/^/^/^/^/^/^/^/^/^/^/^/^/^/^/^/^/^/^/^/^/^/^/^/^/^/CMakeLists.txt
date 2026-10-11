cmake_minimum_required(VERSION 3.16)
project(BoidsPSOLearningLab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(OpenGL REQUIRED)
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# ---- Dear ImGui ----
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.88
)
FetchContent_MakeAvailable(imgui)

# ---- imgui-sfml (source-only; avoids its CMake forcing static SFML) ----
FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v2.5
)
FetchContent_GetProperties(imgui-sfml)
if(NOT imgui-sfml_POPULATED)
    FetchContent_Populate(imgui-sfml)
endif()

if(EXISTS ${imgui-sfml_SOURCE_DIR}/imgui-SFML.cpp)
    set(IMGUI_SFML_CPP ${imgui-sfml_SOURCE_DIR}/imgui-SFML.cpp)
    set(IMGUI_SFML_INC ${imgui-sfml_SOURCE_DIR})
else()
    set(IMGUI_SFML_CPP ${imgui-sfml_SOURCE_DIR}/src/imgui-SFML.cpp)
    set(IMGUI_SFML_INC ${imgui-sfml_SOURCE_DIR}/include)
endif()

add_executable(LearningLab
    src/main.cpp

    # Boids
    src/Boid.cpp
    src/Vector2.cpp
    src/BoidsApp.cpp

    # PSO
    src/pso/PSO.cpp
    src/pso/Rastrigin.cpp
    src/pso/PSOApp.cpp
    src/pso/Objective.cpp 

    # Dear ImGui + backend
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_SFML_CPP}
)

target_include_directories(LearningLab PRIVATE
    include
    include/pso
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${IMGUI_SFML_INC}
)

target_link_libraries(LearningLab
    sfml-graphics
    sfml-window
    sfml-system
    OpenGL::GL
)