import os
import subprocess
import numpy as np
import matplotlib.pyplot as plt

result = subprocess.run(
    ["./build/dipole_sim", "--csv"],
    capture_output=True, text=True,
    cwd=os.path.dirname(os.path.abspath(__file__)) or "."
)
lines = result.stdout.strip().split("\n")

costs = []
section = None

for line in lines:
    if line in ("SENSORS", "FIELDS", "TRUE", "COSTS", "ESTIMATED"):
        section = line
        continue
    if section == "COSTS" and "," not in line and line.strip():
        costs.append(float(line.strip()))

costs = np.array(costs)
iterations = np.arange(1, len(costs) + 1)

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

# left: linear scale
ax1.plot(iterations, costs, 'o-', color='steelblue', linewidth=2, markersize=6)
ax1.set_xlabel('Iteration')
ax1.set_ylabel('Cost  F(x) = 0.5 ||r||²')
ax1.set_title('LM Convergence (linear scale)', fontweight='bold')
ax1.grid(True, alpha=0.3)
ax1.fill_between(iterations, costs, alpha=0.1, color='steelblue')

# right: log scale — shows the exponential convergence clearly
ax2.semilogy(iterations, costs, 'o-', color='crimson', linewidth=2, markersize=6)
ax2.set_xlabel('Iteration')
ax2.set_ylabel('Cost  (log scale)')
ax2.set_title('LM Convergence (log scale)', fontweight='bold')
ax2.grid(True, which='both', alpha=0.3)

plt.suptitle('Levenberg-Marquardt Solver — Cost History', fontsize=13, fontweight='bold')
plt.tight_layout()
plt.savefig('plot3_cost_curve.png', dpi=150)
plt.show()
print("Saved: plot3_cost_curve.png")
