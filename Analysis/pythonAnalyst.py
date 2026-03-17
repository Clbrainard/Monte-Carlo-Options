import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm, LinearSegmentedColormap

# =========================
# 1. Read CSV
# =========================
# CSV format:
# paths,stepsPerHour,meanError,time
df = pd.read_csv(r"tests\ITM_T4.csv")

# =========================
# 2. Aggregate duplicates by taking mean
# =========================
df = df.groupby(['paths', 'stepsPerHour']).agg({
    'meanError': 'mean',
    'time': 'mean'
}).reset_index()

# =========================
# 3. Set column names
# =========================
x_col = "paths"
y_col = "stepsPerHour"
label_col = "meanError"        # shown inside square
color_col = "meanError"        # used for square color
time_display_col = "time"      # shown inside square

# =========================
# 4. Build pivot tables
# =========================
color_pivot = df.pivot(index=y_col, columns=x_col, values=color_col)
error_pivot = df.pivot(index=y_col, columns=x_col, values=label_col)
raw_time_pivot = df.pivot(index=y_col, columns=x_col, values=time_display_col)

color_pivot = color_pivot.sort_index().sort_index(axis=1)
error_pivot = error_pivot.reindex(index=color_pivot.index, columns=color_pivot.columns)
raw_time_pivot = raw_time_pivot.reindex(index=color_pivot.index, columns=color_pivot.columns)

X_labels = color_pivot.columns.values
Y_labels = color_pivot.index.values
Z_color = color_pivot.values
Z_error = error_pivot.values
Z_raw_time = raw_time_pivot.values

# =========================
# 5. Create green -> red colormap
# Green = low error, Red = high error
# =========================
color_cmap = LinearSegmentedColormap.from_list(
    "single_blue",
    ["#eef4ff", "#c6dbff", "#7fb3ff", "#2f7df6", "#0b3d91"]
)
# =========================
# 6. Plot heatmap
# =========================
fig, ax = plt.subplots(figsize=(10, 6))

min_positive = np.nanmin(Z_color[Z_color > 0])
max_val = np.nanmax(Z_color)

im = ax.imshow(
    Z_color,
    origin="lower",
    aspect="auto",
    cmap=color_cmap,
    norm=LogNorm(vmin=min_positive, vmax=max_val)
)

# =========================
# 7. Axis ticks / labels
# =========================
ax.set_xticks(np.arange(len(X_labels)))
ax.set_yticks(np.arange(len(Y_labels)))
ax.set_xticklabels(X_labels)
ax.set_yticklabels(Y_labels)

ax.set_xlabel("paths")
ax.set_ylabel("stepsPerHour")
ax.set_title("Mean Error Color Map with Time Underneath")

# =========================
# 8. Colorbar
# =========================
cbar = plt.colorbar(im, ax=ax)
cbar.set_label("mean error")


# =========================
# 9. Write meanError and time inside each square
# =========================
log_mid = np.sqrt(min_positive * max_val)

for i in range(len(Y_labels)):
    for j in range(len(X_labels)):
        err_val = Z_error[i, j]
        color_val = Z_color[i, j]
        raw_time_val = Z_raw_time[i, j]

        if pd.isna(err_val) or pd.isna(color_val) or pd.isna(raw_time_val):
            continue

        text_color = "white" if color_val > log_mid else "black"

        ax.text(
            j,
            i,
            f"{err_val * 100:.2f}%\n{raw_time_val:.2f}s",
            ha="center",
            va="center",
            color=text_color,
            fontsize=9
        )

plt.tight_layout()
plt.show()