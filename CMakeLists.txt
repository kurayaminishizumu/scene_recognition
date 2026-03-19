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

# 设置 Qt 路径
list(APPEND CMAKE_PREFIX_PATH "E:/Qt/6.10.2/msvc2022_64")

# ==========================================
# 1. 查找依赖包 (针对 2D 图像与网络通信)
# ==========================================
find_package(OpenCV REQUIRED)
# 新增 Network 模块，用于阶段三发送 HTTP 请求给 Python 微服务
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# ------------------------------------------
# 以下库因项目计划调整而延后或转移到后端，暂时注释掉以加快编译和避免报错
# find_package(PCL REQUIRED)
# find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgViewer osgGA osgText osgUtil osg)
find_package(GDAL REQUIRED) 
# find_package(onnxruntime REQUIRED) # 模型推理已移至 Python 后端
# find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets OpenGL) # 不再需要 OpenGL 视口
# ------------------------------------------

# ==========================================
# 2. 定义可执行文件及源文件
# ==========================================
add_executable(${PROJECT_NAME} WIN32
    main.cpp 
    inputoutput.cpp
    imagewidget.cpp
    # 移除了 osgwidget.cpp, pcl2osg.cpp, PointCloudPreprocess.cpp, RansacFilter.cpp
    # 建议后续在此处添加新的数据结构文件，例如：
    # data_structure/ImageData.h
    # data_structure/VectorData.h
)

# ==========================================
# 3. 设置头文件包含目录
# ==========================================
# 注释掉 PCL 相关的宏定义
# add_definitions(${PCL_DEFINITIONS})

target_include_directories(${PROJECT_NAME} PRIVATE 
    # ${PCL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/data_structure
)

# ==========================================
# 4. 链接目标库
# ==========================================
target_link_libraries(${PROJECT_NAME} PRIVATE 
    # 核心库
    ${OpenCV_LIBS} 
    Qt6::Widgets
    Qt6::Network   # 添加网络库用于调用 Python 接口

)