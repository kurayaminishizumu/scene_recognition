import os
import cv2
import numpy as np
import json
import pandas as pd
from PIL import Image
import torch
from transformers import OwlViTProcessor, OwlViTForObjectDetection
from mobile_sam import sam_model_registry, SamPredictor

# ==========================================
# 1. 配置与模型加载
# ==========================================
DEVICE = "cuda" if torch.cuda.is_available() else "cpu"
INPUT_DIR = "./test_images"  # 你的测试图片文件夹
OUTPUT_DIR = "./analysis_results"
PROMPT = "traffic sign"      # 统一测试的提示词
EPSILON_VALUES = [0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0] # 待对比的参数

os.makedirs(OUTPUT_DIR, exist_ok=True)

print(f"[*] 正在加载模型到 {DEVICE}...")
vlm_processor = OwlViTProcessor.from_pretrained("google/owlvit-base-patch32")
vlm_model = OwlViTForObjectDetection.from_pretrained("google/owlvit-base-patch32").to(DEVICE)
sam = sam_model_registry["vit_t"](checkpoint="mobile_sam.pt")
sam.to(device=DEVICE)
sam_predictor = SamPredictor(sam)

def calculate_iou(mask1, mask2):
    intersection = np.logical_and(mask1, mask2).sum()
    union = np.logical_or(mask1, mask2).sum()
    if union == 0: return 1.0
    return intersection / union

def vectorize_and_evaluate(mask, epsilon):
    # 轮廓提取
    contours, _ = cv2.findContours(mask.astype(np.uint8), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours: return None, 0, 0
    
    cnt = contours[0] # 取最大轮廓
    # RDP 算法简化
    approx = cv2.approxPolyDP(cnt, epsilon, True)
    
    # 计算拟合度：将简化后的多边形画回掩膜，与原掩膜对比 IoU
    simplified_mask = np.zeros_like(mask, dtype=np.uint8)
    cv2.drawContours(simplified_mask, [approx], -1, 1, -1)
    
    iou = calculate_iou(mask, simplified_mask)
    vertex_count = len(approx)
    
    return approx, vertex_count, iou

# ==========================================
# 2. 批处理主循环
# ==========================================
data_log = []

image_files = [f for f in os.listdir(INPUT_DIR) if f.endswith(('.jpg', '.png', '.jpeg'))]

for img_name in image_files:
    print(f"[*] 处理图片: {img_name}")
    img_path = os.path.join(INPUT_DIR, img_name)
    img_cv = cv2.imread(img_path)
    img_rgb = cv2.cvtColor(img_cv, cv2.COLOR_BGR2RGB)
    img_pil = Image.fromarray(img_rgb)

    # A. 自动检测 (Owl-ViT)
    inputs = vlm_processor(text=[[PROMPT]], images=img_pil, return_tensors="pt").to(DEVICE)
    with torch.no_grad():
        outputs = vlm_model(**inputs)
    
    target_sizes = torch.Tensor([img_pil.size[::-1]])
    results = vlm_processor.image_processor.post_process_object_detection(outputs, target_sizes=target_sizes, threshold=0.1)[0]
    
    if len(results["boxes"]) == 0:
        print(f"[!] 图片 {img_name} 未检测到目标，跳过")
        continue

    # 取置信度最高的 BBox
    best_idx = results["scores"].argmax()
    bbox = results["boxes"][best_idx].cpu().numpy().astype(int) # [x1, y1, x2, y2]

    # B. 自动分割 (MobileSAM)
    sam_predictor.set_image(img_rgb)
    masks, _, _ = sam_predictor.predict(box=bbox[None, :], multimask_output=False)
    raw_mask = masks[0]

    # C. 遍历 epsilon 参数进行矢量化分析
    for eps in EPSILON_VALUES:
        approx, v_count, iou = vectorize_and_evaluate(raw_mask, eps)
        
        if approx is not None:
            # 记录数据
            data_log.append({
                "image": img_name,
                "epsilon": eps,
                "vertex_count": v_count,
                "shape_fidelity_iou": iou
            })
            
            # 导出 GeoJSON (可选)
            geojson = {
                "type": "Feature",
                "geometry": {
                    "type": "Polygon",
                    "coordinates": [[ [int(p[0][0]), int(p[0][1])] for p in approx ] + [[int(approx[0][0][0]), int(approx[0][0][1])]]]
                },
                "properties": {"epsilon": eps, "iou": iou}
            }
            save_path = os.path.join(OUTPUT_DIR, f"{img_name}_eps{eps}.geojson")
            with open(save_path, 'w') as f:
                json.dump(geojson, f)

# ==========================================
# 3. 导出分析报表
# ==========================================
df = pd.DataFrame(data_log)
df.to_csv(os.path.join(OUTPUT_DIR, "rdp_analysis_report.csv"), index=False)
print(f"[+] 分析完成！报表已保存至 {OUTPUT_DIR}/rdp_analysis_report.csv")
