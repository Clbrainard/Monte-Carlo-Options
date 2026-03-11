import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm, LinearSegmentedColormap

# =========================
# 1. Read CSV
# =========================
# CSV format:
# paths,stepsPerHour,meanError,time
df = pd.read_csv("data.csv")

# =========================
# 2. Scale time by 1800
# =========================
df["scaledTime"] = df["time"] / 1800.0

# =========================
# 3. Set column names
# =========================
x_col = "paths"
y_col = "stepsPerHour"
label_col = "meanError"     # shown inside square
color_col = "scaledTime"    # used for square color

# =========================
# 4. Build pivot tables
# =========================
time_pivot = df.pivot(index=y_col, columns=x_col, values=color_col)
error_pivot = df.pivot(index=y_col, columns=x_col, values=label_col)

time_pivot = time_pivot.sort_index().sort_index(axis=1)
error_pivot = error_pivot.reindex(index=time_pivot.index, columns=time_pivot.columns)

X_labels = time_pivot.columns.values
Y_labels = time_pivot.index.values
Z_time = time_pivot.values
Z_error = error_pivot.values

# =========================
# 5. Create one-color colormap
# =========================
# light -> dark blue
single_color_cmap = LinearSegmentedColormap.from_list(
    "single_blue",
    ["#eef4ff", "#c6dbff", "#7fb3ff", "#2f7df6", "#0b3d91"]
)

# =========================
# 6. Plot heatmap
# =========================
fig, ax = plt.subplots(figsize=(10, 6))

# Log scale makes differences more visible across wide ranges
min_positive = np.nanmin(Z_time[Z_time > 0])
max_val = np.nanmax(Z_time)

im = ax.imshow(
    Z_time,
    origin="lower",
    aspect="auto",
    cmap=single_color_cmap,
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
ax.set_title("Mean Error Labels with Scaled Time Color")

# =========================
# 8. Colorbar
# =========================
cbar = plt.colorbar(im, ax=ax)
cbar.set_label("time / 1800")

# =========================
# 9. Write meanError inside each square
# =========================
log_mid = np.sqrt(min_positive * max_val)

for i in range(len(Y_labels)):
    for j in range(len(X_labels)):
        err_val = Z_error[i, j]
        time_val = Z_time[i, j]

        if pd.isna(err_val) or pd.isna(time_val):
            continue

        ax.text(
            j,
            i,
            f"{err_val:.2f}%",
            ha="center",
            va="center",
            color="white" if time_val > log_mid else "black"
        )

plt.tight_layout()
plt.show()