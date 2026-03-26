import base64
import cv2
import numpy as np
import uvicorn
import torch
import os

# 限制推理只使用 4 个核心，防止把整个系统卡死，为 Qt 留出 UI 刷新资源
torch.set_num_threads(4)

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List
from PIL import Image
from transformers import OwlViTProcessor, OwlViTForObjectDetection

# ==========================================
# 1. 初始化 FastAPI 应用与模型加载
# ==========================================
app = FastAPI(
    title="街景要素提取微服务",
    description="基于 OWL-ViT + SAM 的零样本图像语义分割后端",
    version="1.2.0"
)

# 强制使用 CPU 运行，确保硬件环境稳定
DEVICE = "cpu"
MODEL_ID = "google/owlvit-base-patch32"

print(f"[*] 正在初始化 OWL-ViT 模型 ({MODEL_ID})...")
processor = OwlViTProcessor.from_pretrained(MODEL_ID)
model = OwlViTForObjectDetection.from_pretrained(MODEL_ID).to(DEVICE)
model.eval()
print("[+] 模型加载完毕，服务器准备就绪。")

# ==========================================
# 2. 定义 API 数据交互格式
# ==========================================
class ExtractionRequest(BaseModel):
    image_base64: str = Field(..., description="原始图像 Base64")
    prompt: str = Field(..., description="目标描述，如 'traffic sign'")

class ExtractionResponse(BaseModel):
    status: str
    message: str
    label: str
    confidence: float
    bbox: List[int] # [xmin, ymin, xmax, ymax]
    mask_base64: str

# ==========================================
# 3. 辅助工具函数
# ==========================================
def decode_base64_to_cv2(base64_string: str) -> np.ndarray:
    if "," in base64_string:
        base64_string = base64_string.split(",")[1]
    img_data = base64.b64decode(base64_string)
    np_arr = np.frombuffer(img_data, np.uint8)
    return cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

def encode_cv2_to_base64(image: np.ndarray) -> str:
    _, buffer = cv2.imencode(".png", image)
    return base64.b64encode(buffer).decode("utf-8")

# ==========================================
# 4. API 路由定义
# ==========================================
@app.get("/")
def health_check():
    return {"status": "running", "model": "OWL-ViT", "threads": torch.get_num_threads()}

@app.post("/api/extract_element", response_model=ExtractionResponse)
async def extract_element(request: ExtractionRequest):
    try:
        # 1. 解码前端图像（原始大图）
        img_original = decode_base64_to_cv2(request.image_base64)
        if img_original is None:
            raise ValueError("图像解码失败")
        
        orig_h, orig_w = img_original.shape[:2]
        
        # 2. [关键优化]：下采样以节省内存，并记录缩放比例
        max_inference_width = 800
        if orig_w > max_inference_width:
            scale = max_inference_width / orig_w
            new_w = max_inference_width
            new_h = int(orig_h * scale)
            img_inference = cv2.resize(img_original, (new_w, new_h))
            print(f"[*] 缩放推理图: {orig_w}x{orig_h} -> {new_w}x{new_h} (Scale: {scale:.2f})")
        else:
            scale = 1.0
            img_inference = img_original

        # 转换为模型需要的 PIL 格式
        img_rgb = cv2.cvtColor(img_inference, cv2.COLOR_BGR2RGB)
        img_pil = Image.fromarray(img_rgb)
        
        target_prompt = request.prompt
        print(f"[*] 收到请求 | 目标: '{target_prompt}'")

        # 3. OWL-ViT 推理获取 BBox
        inputs = processor(text=[[target_prompt]], images=img_pil, return_tensors="pt").to(DEVICE)
        
        with torch.no_grad():
            outputs = model(**inputs)
        
        # 4. 后处理：提取坐标
        # 注意：这里的 target_sizes 传入的是【推理图】的尺寸
        target_sizes = torch.Tensor([img_pil.size[::-1]]) 
        results = processor.image_processor.post_process_object_detection(
            outputs=outputs, 
            target_sizes=target_sizes, 
            threshold=0.1 
        )[0]

        boxes = results["boxes"].cpu().numpy()
        scores = results["scores"].cpu().numpy()
        
        if len(boxes) > 0:
            # 找到置信度最高的索引
            best_idx = np.argmax(scores)
            best_score = float(scores[best_idx])
            
            # --- [核心修复：坐标还原] ---
            # 这里的 boxes[best_idx] 是基于 800 像素图的坐标
            # 我们需要将其除以 scale，还原回原图 (1080p 或更大) 的坐标
            raw_box = boxes[best_idx]
            final_box = (raw_box / scale).astype(int).tolist() # [xmin, ymin, xmax, ymax]
            
            print(f"[+] 识别成功: {target_prompt} | 置信度: {best_score:.2f}")
            print(f"    - 推理坐标: {raw_box.astype(int)}")
            print(f"    - 还原坐标: {final_box}")

            # 5. 生成预览掩膜 (基于原始图像尺寸)
            mask_preview = np.zeros((orig_h, orig_w), dtype=np.uint8)
            # 在原图坐标系下画出这个矩形
            cv2.rectangle(mask_preview, 
                          (final_box[0], final_box[1]), 
                          (final_box[2], final_box[3]), 
                          255, -1)

            # 释放不再需要的中间张量，防止内存堆积
            del outputs, inputs

            return ExtractionResponse(
                status="success",
                message="识别成功",
                label=target_prompt,
                confidence=best_score,
                bbox=final_box,
                mask_base64=encode_cv2_to_base64(mask_preview)
            )
        else:
            print(f"[-] 未能识别到: {target_prompt}")
            return ExtractionResponse(
                status="error",
                message="未能在图中找到指定目标",
                label=target_prompt,
                confidence=0.0,
                bbox=[],
                mask_base64=""
            )

    except Exception as e:
        print(f"[!] 错误: {str(e)}")
        raise HTTPException(status_code=500, detail=str(e))

# ==========================================
# 5. 启动入口
# ==========================================
if __name__ == "__main__":
    print("="*50)
    print("OWL-ViT 推理后端已启动")
    print("监听端口: 8000 | 限制 CPU 线程: 4")
    print("="*50)
    uvicorn.run(app, host="0.0.0.0", port=8000)