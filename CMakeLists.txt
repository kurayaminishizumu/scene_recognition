cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0144 NEW) # 消除 PCL/FLANN 警告

project(SceneRecognition)

set(CMAKE_CXX_STANDARD 17)

# 查找库
find_package(OpenCV REQUIRED)
find_package(PCL REQUIRED)
find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgViewer osgGA osgText osgUtil osg)
find_package(GDAL REQUIRED)
find_package(onnxruntime REQUIRED)

# 源文件
add_executable(${PROJECT_NAME} main.cpp)

# 包含 PCL 头文件和定义
add_definitions(${PCL_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PRIVATE ${PCL_INCLUDE_DIRS})

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${OpenCV_LIBS} 
    ${PCL_LIBRARIES} 
    ${OPENSCENEGRAPH_LIBRARIES} 
    GDAL::GDAL
    onnxruntime::onnxruntime    # <-- 改成这样
)