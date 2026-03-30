import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# read csv



def table():
    df = pd.read_csv("tests/ITM_T6.csv")
    regtype_labels = {
        1: "Poly d2",
        2: "Poly d3",
        3: "Legendre d2",
        4: "Legendre d3",
        5: "Hermite d2",
        6: "Hermite d3",
        7: "Laguerre d3"
    }

    steps_per_min = df["stepsPerMin"].iloc[0]

    # pivot
    error_mat = df.pivot(index="paths", columns="Regtype", values="avgSamplePercentError")
    runtime_mat = df.pivot(index="paths", columns="Regtype", values="avgSampleRuntime")

    error_mat = error_mat.sort_index().sort_index(axis=1)
    runtime_mat = runtime_mat.sort_index().sort_index(axis=1)

    # combine into strings
    table_data = []
    for i in range(error_mat.shape[0]):
        row = []
        for j in range(error_mat.shape[1]):
            err = error_mat.iloc[i, j]
            run = runtime_mat.iloc[i, j]
            row.append(f"{err:.2f}%\n{run:.2f}s")
        table_data.append(row)

    # labels
    col_labels = [regtype_labels.get(c, f"Type {c}") for c in error_mat.columns]
    row_labels = error_mat.index.astype(str)

    # plot table
    fig, ax = plt.subplots(figsize=(12, 4))
    ax.axis('off')

    table = ax.table(
        cellText=table_data,
        rowLabels=row_labels,
        colLabels=col_labels,
        cellLoc='center',
        loc='center'
    )

    # styling
    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1.2, 1.8)

    plt.title(f"LSM Regression Results (stepsPerMin = {steps_per_min})\nCell = Error% / Runtime")

    plt.tight_layout()
    plt.show()

def errorGraph():
    df = pd.read_csv("tests/ITM_T5.csv")
    regtype_labels = {
        1: "Poly d2",
        2: "Poly d3",
        3: "Legendre d2",
        4: "Legendre d3",
        5: "Hermite d2",
        6: "Hermite d3",
        7: "Laguerre d3"
    }
    
    steps_per_min = df["stepsPerMin"].iloc[0]

    df = df.sort_values(by=["Regtype", "paths"])

    plt.figure(figsize=(8, 5))

    for reg in sorted(df["Regtype"].unique()):
        subset = df[df["Regtype"] == reg]
        
        x = subset["paths"].to_numpy()
        y = subset["avgSamplePercentError"].to_numpy()
        
        plt.plot(x, y, marker='o', label=regtype_labels.get(reg, f"Type {reg}"))

    #plt.xscale("log")
    plt.xlabel("Number of Paths (log scale)")
    plt.ylabel("Average Percent Error (%)")
    plt.title(f"Error vs Paths by Regression Type (stepsPerMin = {steps_per_min})")

    plt.legend()
    plt.grid(True, which="both", linestyle="--", linewidth=0.5)

    plt.tight_layout()
    plt.show()

table()
