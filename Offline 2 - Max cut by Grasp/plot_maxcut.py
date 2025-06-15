import pandas as pd
import matplotlib.pyplot as plt

# Load your CSV file
df = pd.read_csv("2105106.csv")

# Filter for Graphs G1 to G10
df_10 = df[df['Name'].str.contains(r'^G(10|[1-9])$', regex=True)]

# Extract relevant columns
graphs = df_10['Name']
randomized = df_10['Simple Randomized or Randomized 1']
greedy = df_10['Simple Greedy or Greedy 1']
semi_greedy = df_10['Semi-Greedy 1']
local = df_10['LocalSearch-Average value']
grasp = df_10['GRASP-Best Value']

# Plot setup
x = range(len(graphs))
bar_width = 0.15

plt.figure(figsize=(14, 6))
plt.bar([i - 2*bar_width for i in x], randomized, width=bar_width, label='Randomized', color='skyblue')
plt.bar([i - bar_width for i in x], greedy, width=bar_width, label='Greedy', color='orange')
plt.bar(x, semi_greedy, width=bar_width, label='Semi-Greedy', color='violet')
plt.bar([i + bar_width for i in x], grasp, width=bar_width, label='GRASP', color='gold')
plt.bar([i + 2*bar_width for i in x], local, width=bar_width, label='Local Search', color='deepskyblue')

# Labels and formatting
plt.xticks(x, graphs)
plt.xlabel('Graph')
plt.ylabel('Max Cut Value')
plt.title('Max Cut (Graph 1-10)')
plt.legend()
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()

# Save or show
plt.savefig("maxcut_graphs_1_to_10.png")
plt.show()
