import pandas as pd
import matplotlib.pyplot as plt

criteria = ["IG", "IGR", "NWIG"]
colors = {"IG": "blue", "IGR": "green", "NWIG": "red"}

# Load and plot accuracy vs depth
plt.figure(figsize=(10, 6))
for crit in criteria:
    df = pd.read_csv(f"(IRIS)accuracy_vs_depth_{crit}.csv")
    plt.plot(df["depth"], df["avg_accuracy"], marker='o', label=crit, color=colors[crit])

plt.title("Average Accuracy vs Max Tree Depth")
plt.xlabel("Max Tree Depth")
plt.ylabel("Average Accuracy (%)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("(IRIS)accuracy_vs_depth.png")
plt.show()

# Load and plot node count vs depth
plt.figure(figsize=(10, 6))
for crit in criteria:
    df = pd.read_csv(f"(IRIS)accuracy_vs_depth_{crit}.csv")
    plt.plot(df["depth"], df["avg_node_count"], marker='o', label=crit, color=colors[crit])

plt.title("Average Node Count vs Max Tree Depth")
plt.xlabel("Max Tree Depth")
plt.ylabel("Average Node Count")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("(IRIS)node_count_vs_depth.png")
plt.show()
