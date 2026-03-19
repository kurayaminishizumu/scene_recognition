import base64
import cv2
import numpy as np
import uvicorn
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List

# ==========================================
# 1. 初始化 FastAPI 应用
# ==========================================
app = FastAPI(
    title="街景要素提取微服务",
    description="基于 VLM + SAM 的零样本图像语义分割后端",
    version="1.0.0"
)

# ==========================================
# 2. 定义 API 数据交互格式 (Pydantic 模型)
# ==========================================
class ExtractionRequest(BaseModel):
    """前端 (C++) 发送的请求体结构"""
    image_base64: str = Field(..., description="原始街景图像的 Base64 编码字符串 (JPEG/PNG格式)")
    prompt: str = Field(..., description="要提取的要素自然语言描述，例如 'traffic sign', 'window'")

class ExtractionResponse(BaseModel):
    """后端返回给前端 (C++) 的响应体结构"""
    status: str = Field(..., description="请求状态：'success' 或 'error'")
    message: str = Field(default="", description="附加信息或错误提示")
    label: str = Field(default="", description="识别出的要素类别")
    confidence: float = Field(default=0.0, description="VLM 识别置信度 (0.0 ~ 1.0)")
    bbox: List[int] = Field(default_factory=list, description="边界框 [x, y, width, height]")
    mask_base64: str = Field(default="", description="单通道二值 Mask 图像的 Base64 编码字符串")

# ==========================================
# 3. 辅助工具函数
# ==========================================
def decode_base64_to_cv2(base64_string: str) -> np.ndarray:
    """将 Base64 字符串解码为 OpenCV 图像 (cv2.Mat / numpy array)"""
    try:
        # 移除可能存在的前缀 (如 "data:image/jpeg;base64,")
        if "," in base64_string:
            base64_string = base64_string.split(",")[1]
            
        img_data = base64.b64decode(base64_string)
        np_arr = np.frombuffer(img_data, np.uint8)
        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
        if img is None:
            raise ValueError("图像解码失败，数据损坏或格式不支持")
        return img
    except Exception as e:
        raise ValueError(f"Base64 解码异常: {str(e)}")

def encode_cv2_to_base64(image: np.ndarray, ext: str = ".png") -> str:
    """将 OpenCV 图像编码为 Base64 字符串"""
    success, buffer = cv2.imencode(ext, image)
    if not success:
        raise ValueError("图像编码失败")
    return base64.b64encode(buffer).decode("utf-8")

# ==========================================
# 4. API 路由定义
# ==========================================
@app.get("/")
def health_check():
    """健康检查接口，用于测试服务器是否启动"""
    return {"status": "running", "service": "VLM-SAM-Backend"}

@app.post("/api/extract_element", response_model=ExtractionResponse)
async def extract_element(request: ExtractionRequest):
    """
    核心接口：接收图像与指令，返回 BBox 与 Mask
    """
    try:
        # 1. 解析前端传来的 Base64 图像
        img = decode_base64_to_cv2(request.image_base64)
        target_prompt = request.prompt
        
        print(f"[*] 收到提取请求 | 目标: '{target_prompt}' | 图像尺寸: {img.shape}")

        # -----------------------------------------------------------------
        # [预留位置：任务 2.2 & 2.3]
        # 这里是您后续接入 Ollama (VLM) 和 SAM (ONNX) 的核心逻辑区
        # 
        # 步骤 A: 调用 Ollama (Qwen-VL) 获取 BBox
        # bbox = call_ollama_for_bbox(img, target_prompt)
        #
        # 步骤 B: 将图像和 BBox 输入 SAM 模型获取 Mask
        # mask = call_sam_for_mask(img, bbox)
        # -----------------------------------------------------------------

        # [模拟执行]：为了让你现阶段能跑通前后端通信，这里生成一个伪造的结果
        # 假设 VLM 找到了一个区域 (x=100, y=100, w=200, h=200)
        fake_bbox = [100, 100, 200, 200]
        
        # 假设 SAM 生成了一个白色的圆形掩码
        fake_mask = np.zeros(img.shape[:2], dtype=np.uint8)
        cv2.circle(fake_mask, (200, 200), 100, 255, -1)
        
        # 2. 将 Mask 图像重新编码为 Base64 准备返回给 C++
        mask_b64 = encode_cv2_to_base64(fake_mask, ext=".png")

        # 3. 构造并返回 JSON 响应
        return ExtractionResponse(
            status="success",
            message="要素提取成功 (当前为模拟数据)",
            label=target_prompt,
            confidence=0.98,
            bbox=fake_bbox,
            mask_base64=mask_b64
        )

    except ValueError as ve:
        # 处理图像解码等已知错误
        raise HTTPException(status_code=400, detail=str(ve))
    except Exception as e:
        # 处理模型推理等未知错误
        print(f"[!] 服务器内部错误: {str(e)}")
        raise HTTPException(status_code=500, detail="服务器内部推理错误")

# ==========================================
# 5. 启动入口
# ==========================================
if __name__ == "__main__":
    # 运行服务器 (默认监听 0.0.0.0:8000)
    print("="*50)
    print("启动街景要素提取微服务...")
    print("API 接口文档地址: http://127.0.0.1:8000/docs")
    print("="*50)
    uvicorn.run(app, host="0.0.0.0", port=8000)