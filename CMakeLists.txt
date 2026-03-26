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
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(GDAL REQUIRED) 

# ==========================================
# 2. 定义可执行文件及源文件
# ==========================================
add_executable(${PROJECT_NAME} WIN32
    main.cpp 
    inputoutput.cpp
    imagewidget.cpp
)

# ==========================================
# 3. 设置头文件包含目录
# ==========================================

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/data_structure
)

# ==========================================
# 4. 链接目标库
# ==========================================
target_link_libraries(${PROJECT_NAME} PRIVATE 
    # 核心库
    ${OpenCV_LIBS} 
    Qt6::Widgets
    Qt6::Network

)