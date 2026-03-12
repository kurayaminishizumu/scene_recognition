cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0144 NEW) 

project(SceneRecognition)
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()
set(CMAKE_CXX_STANDARD 17)

# 1. 开启 Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 2. 告诉 CMake 你的 Qt 6 安装路径 (手动安装必须加这一行)
list(APPEND CMAKE_PREFIX_PATH "E:/Qt/6.10.2/msvc2022_64")

# 3. 查找库
find_package(OpenCV REQUIRED)
find_package(PCL REQUIRED)
find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgViewer osgGA osgText osgUtil osg)
find_package(GDAL REQUIRED)
find_package(onnxruntime REQUIRED)

# ====== 关键修改：查找 Qt6 模块 ======
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets OpenGL)
# ===================================

add_executable(${PROJECT_NAME} main.cpp)

add_definitions(${PCL_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PRIVATE ${PCL_INCLUDE_DIRS})

# 4. 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${OpenCV_LIBS} 
    ${PCL_LIBRARIES} 
    ${OPENSCENEGRAPH_LIBRARIES} 
    GDAL::GDAL
    onnxruntime::onnxruntime
    
    # ====== 关键修改：链接 Qt6 目标 ======
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::OpenGL
    # ===================================
)