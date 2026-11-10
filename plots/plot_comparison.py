import os
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

result = subprocess.run(
    ["./build/dipole_sim", "--csv"],
    capture_output=True, text=True,
    cwd=os.path.dirname(os.path.abspath(__file__)) or "."
)
lines = result.stdout.strip().split("\n")

sensors, true_pos, true_mom, est_pos, est_mom = [], None, None, None, None
section = None
true_count = 0
est_count  = 0

for line in lines:
    if line in ("SENSORS", "FIELDS", "TRUE", "COSTS", "ESTIMATED"):
        section = line
        continue
    if "," not in line:
        continue
    vals = list(map(float, line.split(",")))
    if section == "SENSORS":
        sensors.append(vals)
    elif section == "TRUE":
        if true_count == 0: true_pos = vals
        else:               true_mom = vals
        true_count += 1
    elif section == "ESTIMATED":
        if est_count == 0: est_pos = vals
        else:              est_mom = vals
        est_count += 1

sensors  = np.array(sensors)
true_pos = np.array(true_pos)
true_mom = np.array(true_mom)
est_pos  = np.array(est_pos)
est_mom  = np.array(est_mom)

fig = plt.figure(figsize=(10, 7))
ax  = fig.add_subplot(111, projection='3d')

# sensors
ax.scatter(sensors[:,0], sensors[:,1], sensors[:,2],
           color='steelblue', s=40, alpha=0.6, label='Sensors')

# true dipole — position + moment arrow
ax.scatter(*true_pos, color='crimson', s=250, marker='*', zorder=10, label='True dipole')
scale = 0.03
ax.quiver(*true_pos,
          true_mom[0]*scale, true_mom[1]*scale, true_mom[2]*scale,
          color='crimson', linewidth=2.5, arrow_length_ratio=0.3)

# estimated dipole — position + moment arrow
ax.scatter(*est_pos, color='limegreen', s=150, marker='^', zorder=10, label='Estimated dipole')
ax.quiver(*est_pos,
          est_mom[0]*scale, est_mom[1]*scale, est_mom[2]*scale,
          color='limegreen', linewidth=2.5, arrow_length_ratio=0.3)

# line connecting true to estimated
ax.plot([true_pos[0], est_pos[0]],
        [true_pos[1], est_pos[1]],
        [true_pos[2], est_pos[2]],
        'k--', linewidth=1.2, alpha=0.5, label='Error')

# error annotation
err_mm = np.linalg.norm(true_pos - est_pos) * 1e3
mid = (true_pos + est_pos) / 2
ax.text(mid[0], mid[1], mid[2]+0.005,
        f'Δ = {err_mm:.4f} mm', fontsize=9, color='black')

ax.set_xlabel('X (m)')
ax.set_ylabel('Y (m)')
ax.set_zlabel('Z (m)')
ax.set_title('True vs Estimated Dipole Position & Moment',
             fontsize=13, fontweight='bold')
ax.legend()
plt.tight_layout()
plt.savefig('plot4_comparison.png', dpi=150)
plt.show()
print("Saved: plot4_comparison.png")
