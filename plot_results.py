import pandas as pd
import matplotlib.pyplot as plt
import os

# ==========================================
# 1. 加载数据
# ==========================================
CSV_PATH = "./analysis_results/rdp_analysis_report.csv"
OUTPUT_PLOT = "./analysis_results/rdp_performance_analysis.png"

if not os.path.exists(CSV_PATH):
    print(f"[!] 找不到文件: {CSV_PATH}，请先运行 batch_analysis.py")
    exit()

df = pd.read_csv(CSV_PATH)

# ==========================================
# 2. 数据聚合 (按 epsilon 计算均值和标准差)
# ==========================================
stats = df.groupby('epsilon').agg({
    'vertex_count': ['mean', 'std'],
    'shape_fidelity_iou': ['mean', 'std']
}).reset_index()

# 简化列名
stats.columns = ['epsilon', 'v_mean', 'v_std', 'iou_mean', 'iou_std']

# ==========================================
# 3. 绘图配置
# ==========================================
plt.rcParams['font.sans-serif'] = ['Arial'] # 论文常用字体
plt.rcParams['axes.unicode_minus'] = False
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

# --- 图表 1: Epsilon vs. Vertex Count (顶点压缩效率) ---
ax1.plot(stats['epsilon'], stats['v_mean'], marker='o', color='#1f77b4', linewidth=2, label='Mean Vertex Count')
ax1.fill_between(stats['epsilon'], stats['v_mean'] - stats['v_std'], stats['v_mean'] + stats['v_std'], 
                 color='#1f77b4', alpha=0.2, label='Standard Deviation')
ax1.set_title('RDP Epsilon vs. Vertex Count', fontsize=14, fontweight='bold')
ax1.set_xlabel('Epsilon (Tolerance)', fontsize=12)
ax1.set_ylabel('Number of Vertices', fontsize=12)
ax1.grid(True, linestyle='--', alpha=0.7)
ax1.legend()

# --- 图表 2: Epsilon vs. IoU (形状拟合保真度) ---
ax2.plot(stats['epsilon'], stats['iou_mean'], marker='s', color='#d62728', linewidth=2, label='Mean IoU')
ax2.fill_between(stats['epsilon'], stats['iou_mean'] - stats['iou_std'], stats['iou_mean'] + stats['iou_std'], 
                 color='#d62728', alpha=0.2, label='Standard Deviation')
ax2.set_title('RDP Epsilon vs. Shape Fidelity (IoU)', fontsize=14, fontweight='bold')
ax2.set_xlabel('Epsilon (Tolerance)', fontsize=12)
ax2.set_ylabel('Intersection over Union (IoU)', fontsize=12)
ax2.set_ylim(0, 1.05) # IoU 最大为 1
ax2.grid(True, linestyle='--', alpha=0.7)
ax2.legend()

plt.tight_layout()

# 保存并显示
plt.savefig(OUTPUT_PLOT, dpi=300) # 高分辨率保存，适合插入论文
print(f"[+] 统计图表已保存至: {OUTPUT_PLOT}")
plt.show()
