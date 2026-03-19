import base64
import cv2
import numpy as np
import uvicorn
import torch
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List
from PIL import Image
import io
from transformers import OwlViTProcessor, OwlViTForObjectDetection

# ==========================================
# 1. 初始化 FastAPI 应用与模型加载
# ==========================================
app = FastAPI(
    title="街景要素提取微服务",
    description="基于 OWL-ViT + SAM 的零样本图像语义分割后端",
    version="1.1.0"
)

# 强制使用 CPU 运行，确保 2G 显存环境绝对稳定
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
    return {"status": "running", "model": "OWL-ViT"}

@app.post("/api/extract_element", response_model=ExtractionResponse)
async def extract_element(request: ExtractionRequest):
    try:
        # 1. 解码前端图像
        img_cv = decode_base64_to_cv2(request.image_base64)
        if img_cv is None:
            raise ValueError("图像解码失败")
        
        # 将 OpenCV (BGR) 转为 PIL (RGB) 供模型使用
        img_rgb = cv2.cvtColor(img_cv, cv2.COLOR_BGR2RGB)
        img_pil = Image.fromarray(img_rgb)
        
        target_prompt = request.prompt
        print(f"[*] 收到请求 | 目标: '{target_prompt}'")

        # 2. OWL-ViT 推理获取 BBox
        inputs = processor(text=[[target_prompt]], images=img_pil, return_tensors="pt").to(DEVICE)
        
        with torch.no_grad():
            outputs = model(**inputs)
        
        # 3. 后处理：转换坐标
        target_sizes = torch.Tensor([img_pil.size[::-1]]) # [h, w]
        results = processor.image_processor.post_process_object_detection(
            outputs=outputs, 
            target_sizes=target_sizes, 
            threshold=0.1 # 这里的阈值可以根据需要调整
        )[0]

        # 4. 提取结果
        boxes = results["boxes"].cpu().numpy()
        scores = results["scores"].cpu().numpy()
        
        if len(boxes) > 0:
            # 找到置信度最高的索引
            best_idx = np.argmax(scores)
            best_box = boxes[best_idx].astype(int).tolist() # [xmin, ymin, xmax, ymax]
            best_score = float(scores[best_idx])
            
            print(f"[+] 识别成功: {target_prompt} | 置信度: {best_score:.2f} | 坐标: {best_box}")

            # ---------------------------------------------------------
            # [占位] 以后这里接入 MobileSAM 推理 Mask
            # 目前暂时返回一个基于 BBox 的模拟矩形 Mask 供前端测试
            mask_preview = np.zeros(img_cv.shape[:2], dtype=np.uint8)
            cv2.rectangle(mask_preview, (best_box[0], best_box[1]), (best_box[2], best_box[3]), 255, -1)
            # ---------------------------------------------------------

            return ExtractionResponse(
                status="success",
                message="识别成功",
                label=target_prompt,
                confidence=best_score,
                bbox=best_box,
                mask_base64=encode_cv2_to_base64(mask_preview)
            )
        else:
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
    # 建议使用固定的端口，方便 C++ 前端调用
    uvicorn.run(app, host="0.0.0.0", port=8000)