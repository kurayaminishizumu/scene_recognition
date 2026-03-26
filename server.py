import base64
import cv2
import numpy as np
import uvicorn
import torch
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List
from PIL import Image
from transformers import OwlViTProcessor, OwlViTForObjectDetection
# --- 导入 MobileSAM ---
from mobile_sam import sam_model_registry, SamPredictor

# 限制 CPU 线程，保证系统不卡死
torch.set_num_threads(4)

app = FastAPI(title="街景要素提取微服务 V1.3")

# ==========================================
# 1. 全局模型加载 (VLM + SAM)
# ==========================================
DEVICE = "cpu"

# A. 加载 OWL-ViT
print("[*] 正在加载 OWL-ViT...")
vlm_processor = OwlViTProcessor.from_pretrained("google/owlvit-base-patch32")
vlm_model = OwlViTForObjectDetection.from_pretrained("google/owlvit-base-patch32").to(DEVICE)
vlm_model.eval()

# B. 加载 MobileSAM
print("[*] 正在加载 MobileSAM...")
sam_checkpoint = "mobile_sam.pt"
model_type = "vit_t" # MobileSAM 专用架构
sam = sam_model_registry[model_type](checkpoint=sam_checkpoint)
sam.to(device=DEVICE)
sam_predictor = SamPredictor(sam)

print("[+] 所有模型加载完毕，服务器就绪。")

# ==========================================
# 2. 数据模型定义
# ==========================================
class ExtractionRequest(BaseModel):
    image_base64: str
    prompt: str = ""
    bbox: List[int] = [] # 供 SAM 接口使用

class ExtractionResponse(BaseModel):
    status: str
    message: str
    label: str
    confidence: float # 这里可以保留，表示最高置信度，或者改为 confidences 列表
    bboxes: List[List[int]] = [] # [ [x1,y1,x2,y2], [x1,y1,x2,y2], ... ]
    mask_base64: str = "" # SAM 接口目前依然返回单图

# ==========================================
# 3. 辅助函数
# ==========================================
def decode_base64_to_cv2(base64_string: str) -> np.ndarray:
    if "," in base64_string: base64_string = base64_string.split(",")[1]
    img_data = base64.b64decode(base64_string)
    np_arr = np.frombuffer(img_data, np.uint8)
    return cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

def encode_cv2_to_base64(image: np.ndarray) -> str:
    _, buffer = cv2.imencode(".png", image)
    return base64.b64encode(buffer).decode("utf-8")

# ==========================================
# 4. API 路由
# ==========================================

# --- 接口 1: VLM 获取 BBox ---
@app.post("/api/extract_element", response_model=ExtractionResponse)
async def extract_element(request: ExtractionRequest):
    try:
        # 1. 解码图像并记录原始尺寸
        img_original = decode_base64_to_cv2(request.image_base64)
        if img_original is None: raise ValueError("图像解码失败")
        orig_h, orig_w = img_original.shape[:2]

        # 2. 下采样（缩放图片以节省内存）
        max_w = 800
        scale = max_w / orig_w if orig_w > max_w else 1.0
        img_inference = cv2.resize(img_original, (int(orig_w*scale), int(orig_h*scale)))
        
        # 3. OWL-ViT 推理
        img_pil = Image.fromarray(cv2.cvtColor(img_inference, cv2.COLOR_BGR2RGB))
        inputs = vlm_processor(text=[[request.prompt]], images=img_pil, return_tensors="pt").to(DEVICE)
        
        with torch.no_grad():
            outputs = vlm_model(**inputs)
        
        # 4. 后处理：获取原始检测结果
        target_sizes = torch.Tensor([img_pil.size[::-1]])
        results = vlm_processor.image_processor.post_process_object_detection(
            outputs=outputs, target_sizes=target_sizes, threshold=0.0 # 阈值设为0，手动过滤
        )[0]

        boxes = results["boxes"].cpu().numpy()
        scores = results["scores"].cpu().numpy()
        
        # --- 核心逻辑：多目标 NMS 过滤 ---
        final_bboxes = []
        highest_score = 0.0

        if len(boxes) > 0:
            # A. 初步筛选：保留置信度 > 0.08 的框
            score_threshold = 0.08
            keep_idx = np.where(scores > score_threshold)[0]
            
            if len(keep_idx) > 0:
                filtered_boxes = boxes[keep_idx]
                filtered_scores = scores[keep_idx]
                
                # B. 转换格式供 NMS 使用: [x1, y1, x2, y2] -> [x, y, w, h]
                nms_input_boxes = []
                for b in filtered_boxes:
                    nms_input_boxes.append([int(b[0]), int(b[1]), int(b[2]-b[0]), int(b[3]-b[1])])
                
                # C. 执行 NMS (非极大值抑制)，去除重叠过高的框
                # nms_threshold=0.3 表示如果两个框重叠度超过 30%，只留置信度高的那个
                indices = cv2.dnn.NMSBoxes(
                    bboxes=nms_input_boxes, 
                    scores=filtered_scores.tolist(), 
                    score_threshold=score_threshold, 
                    nms_threshold=0.3
                )
                
                # D. 提取 NMS 后的结果并还原比例
                if len(indices) > 0:
                    for i in indices.flatten():
                        raw_box = filtered_boxes[i]
                        # 坐标还原回原图尺寸
                        restored_box = (raw_box / scale).astype(int).tolist()
                        final_bboxes.append(restored_box)
                        # 记录最高分
                        if filtered_scores[i] > highest_score:
                            highest_score = float(filtered_scores[i])

        # 5. 返回响应
        if len(final_bboxes) > 0:
            print(f"[+] 识别成功: 找到 {len(final_bboxes)} 个目标")
            return ExtractionResponse(
                status="success",
                message=f"Success: found {len(final_bboxes)} objects",
                label=request.prompt,
                confidence=highest_score,
                bboxes=final_bboxes, # 这里返回所有找到的矩形框列表
                mask_base64=""      # 此时暂不返回 Mask
            )
        else:
            return ExtractionResponse(
                status="error",
                message="No objects found",
                label=request.prompt,
                confidence=0.0,
                bboxes=[],
                mask_base64=""
            )

    except Exception as e:
        print(f"[!] 后端报错: {e}")
        raise HTTPException(status_code=500, detail=str(e))
# --- 接口 2: SAM 获取 Mask (核心新增) ---
@app.post("/api/run_sam", response_model=ExtractionResponse)
async def run_sam(request: ExtractionRequest):
    try:
        # 1. 解码图像
        img_cv = decode_base64_to_cv2(request.image_base64)
        img_rgb = cv2.cvtColor(img_cv, cv2.COLOR_BGR2RGB)
        
        # 2. 准备 BBox
        if not request.bbox or len(request.bbox) != 4:
            raise ValueError("未接收到有效的 BBox 坐标")
        input_box = np.array(request.bbox) # [xmin, ymin, xmax, ymax]

        # 3. SAM 推理
        print(f"[*] SAM 开始推理 Mask | BBox: {input_box}")
        sam_predictor.set_image(img_rgb)
        # box 格式需符合 [xmin, ymin, xmax, ymax]
        masks, scores, _ = sam_predictor.predict(
            point_coords=None,
            point_labels=None,
            box=input_box[None, :], # 增加维度变成 [1, 4]
            multimask_output=False,
        )
        
        # 4. 将 Mask (True/False) 转为图片 (0/255)
        mask_uint8 = (masks[0] * 255).astype(np.uint8)
        
        # 5. 返回结果
        return ExtractionResponse(
            status="success",
            message="Mask 分割成功",
            label="mask_only",    # 补上缺失的 label
            confidence=1.0,       # 补上缺失的 confidence
            bboxes=[],            # 补上缺失的 bboxes
            mask_base64=encode_cv2_to_base64(mask_uint8)
        )
    except Exception as e:
        print(f"[!] SAM 错误: {e}")
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)