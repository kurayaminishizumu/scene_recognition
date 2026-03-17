cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0144 NEW) 

project(SceneRecognition)
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_PREFIX_PATH "E:/Qt/6.10.2/msvc2022_64")

find_package(OpenCV REQUIRED)
find_package(PCL REQUIRED)
find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgViewer osgGA osgText osgUtil osg)
find_package(GDAL REQUIRED)
find_package(onnxruntime REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets OpenGL)

add_executable(${PROJECT_NAME} WIN32
    main.cpp 
    inputoutput.cpp
    osgwidget.cpp 
    pcl2osg.cpp
    PointCloudPreprocess.cpp 
)

add_definitions(${PCL_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${PCL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/data_structure
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${OpenCV_LIBS} 
    ${PCL_LIBRARIES} 
    ${OPENSCENEGRAPH_LIBRARIES} 
    GDAL::GDAL
    onnxruntime::onnxruntime
    
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::OpenGL
)