# 城市街景要素智能化处理系统 (Scene Recognition App)

本项目是一个集成了前沿深度学习模型与传统计算机视觉算法的跨平台应用，旨在实现城市街景图像中要素的**零样本自动识别、高精度像素分割及结构化矢量化导出**。

本系统特别适用于城市规划、GIS 数据采集以及计算机视觉相关的学术研究（如本科毕业论文课题）。

---

## 🌟 核心特性

- **零样本目标检测 (Zero-shot Detection)**：集成 **Owl-ViT** 大模型，无需预训练即可通过自然语言提示词（如 "traffic sign", "tree"）识别街景要素。
- **高精度交互式分割 (Interactive Segmentation)**：集成 **MobileSAM**，实现点击式的像素级掩码（Mask）生成。
- **智能矢量化模块**：基于 **OpenCV 轮廓提取**与 **RDP (Ramer-Douglas-Peucker) 算法**，实现边界顶点的智能稀疏化，保持拓扑特征的同时极大压缩数据量。
- **标准 GIS 数据导出**：支持将识别结果一键导出为通用的 **GeoJSON** 矢量格式。
- **自动化科研分析流水线**：内置批量处理与量化评估脚本，支持自动计算 **IoU (交并比)** 与**顶点压缩率**，并生成学术统计图表。

---

## 🏗️ 系统架构

系统采用分布式跨语言架构设计：

- **前端 (Client)**: 基于 **C++ 17** 与 **Qt 6** 开发。负责图形渲染、坐标变换、矢量化算法执行及用户交互。
- **后端 (Server)**: 基于 **Python 3.9+** 与 **FastAPI** 开发。负责运行 PyTorch 深度学习模型（Owl-ViT & MobileSAM）并提供 RESTful API。
- **通信**: 采用 HTTP/JSON 协议进行 Base64 图像数据与坐标信息的异步传输。

---

## 🚀 快速开始

### 1. 环境准备

#### 后端 (Python)
```bash
cd scene_recognition
pip install torch torchvision transformers fastapi uvicorn opencv-python pillow pandas matplotlib
# 确保下载 mobile_sam.pt 权重文件并放置在根目录
```

#### 前端 (C++/Qt)
- **依赖管理**: 推荐使用 `vcpkg` 安装 `opencv4`, `qt6-base`, `qt6-network`, `gdal`。
- **构建工具**: CMake 3.15+。
- **编译器**: MSVC 2022 (Windows) 或 GCC/Clang。

### 2. 运行系统

1. **启动后端服务**:
   ```bash
   python server.py
   ```
   服务器默认运行在 `http://127.0.0.1:8000`。

2. **编译并运行前端**:
   使用 CMake 配置并生成工程，启动 `SceneRecognition.exe`。

---

## 📊 学术分析模块

本项目内置了完整的算法性能分析工具，用于定量研究矢量化参数对结果的影响：

1. **批量数据采集**:
   将测试图片放入 `test_images` 文件夹，运行：
   ```bash
   python batch_analysis.py
   ```
   脚本将自动完成检测、分割、多参数矢量化，并计算 **IoU 拟合度**。

2. **生成统计图表**:
   运行：
   ```bash
   python plot_results.py
   ```
   系统将自动生成 `Epsilon vs. Vertex Count` 和 `Epsilon vs. IoU` 的学术折线图，保存在 `analysis_results` 目录。

---

## 📂 项目结构

```text
├── scene_recognition/
│   ├── main.cpp            # 主窗口逻辑与网络交互
│   ├── imagewidget.cpp     # 自定义图像渲染与交互组件
│   ├── vectorization.cpp   # OpenCV/RDP 矢量化核心算法
│   ├── inputoutput.cpp     # GeoJSON 序列化与文件 IO
│   ├── server.py           # FastAPI 模型推理后端
│   ├── batch_analysis.py   # 自动化性能评估脚本
│   ├── plot_results.py     # 学术图表生成脚本
│   ├── CMakeLists.txt      # 项目构建配置
│   └── vcpkg.json          # C++ 依赖管理
└── README.md               # 项目说明文档
```

---

## 📜 免责声明

本项目生成的安全规则与算法参数仅供原型开发与学术研究使用。在部署于生产环境前，请务必进行全面的安全审查与参数调优。

---
**作者**: [qiuzhenlai]  
**项目地址**: [https://github.com/kurayaminishizumu/scene_recognition]
