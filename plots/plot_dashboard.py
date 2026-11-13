import os
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# ── get data from C++ ─────────────────────────────────────────────────────
result = subprocess.run(
    ["./build/dipole_sim", "--csv"],
    capture_output=True, text=True,
    cwd=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
)
lines = result.stdout.strip().split("\n")

sensors, fields, costs = [], [], []
true_pos, true_mom, est_pos, est_mom = None, None, None, None
section = None
true_count = est_count = 0

for line in lines:
    if line in ("SENSORS","FIELDS","TRUE","COSTS","ESTIMATED"):
        section = line
        continue
    if section == "COSTS" and "," not in line and line.strip():
        costs.append(float(line.strip()))
        continue
    if "," not in line:
        continue
    vals = list(map(float, line.split(",")))
    if section == "SENSORS":   sensors.append(vals)
    elif section == "FIELDS":  fields.append(vals)
    elif section == "TRUE":
        if true_count == 0: true_pos = vals
        else:               true_mom = vals
        true_count += 1
    elif section == "ESTIMATED":
        if est_count == 0: est_pos = vals
        else:              est_mom = vals
        est_count += 1

sensors  = np.array(sensors)
fields   = np.array(fields)
costs    = np.array(costs)
true_pos = np.array(true_pos)
true_mom = np.array(true_mom)
est_pos  = np.array(est_pos)
est_mom  = np.array(est_mom)

# ── figure layout ─────────────────────────────────────────────────────────
fig = plt.figure(figsize=(16, 12))
fig.suptitle('Magnetic Dipole Field Simulator', fontsize=16, fontweight='bold', y=0.98)

# ── Plot 1: sensor geometry ───────────────────────────────────────────────
ax1 = fig.add_subplot(2, 2, 1, projection='3d')
ax1.scatter(sensors[:,0], sensors[:,1], sensors[:,2],
            color='steelblue', s=50, label='Sensors')
for s in sensors:
    ax1.plot([s[0],s[0]], [s[1],s[1]], [0, s[2]],
             color='steelblue', alpha=0.15, linewidth=0.8)
ax1.scatter(*true_pos, color='crimson', s=200, marker='*', label='True dipole')
ax1.set_xlabel('X (m)'); ax1.set_ylabel('Y (m)'); ax1.set_zlabel('Z (m)')
ax1.set_title('1. Sensor Array Geometry', fontweight='bold')
ax1.legend(fontsize=8)

# ── Plot 2: field vectors ─────────────────────────────────────────────────
ax2 = fig.add_subplot(2, 2, 2, projection='3d')
mags = np.linalg.norm(fields, axis=1, keepdims=True)
fn   = fields / mags.max() * 0.025
ax2.quiver(sensors[:,0], sensors[:,1], sensors[:,2],
           fn[:,0], fn[:,1], fn[:,2],
           color='steelblue', linewidth=1.5, arrow_length_ratio=0.3)
ax2.scatter(sensors[:,0], sensors[:,1], sensors[:,2],
            color='steelblue', s=30)
ax2.scatter(*true_pos, color='crimson', s=200, marker='*', label='True dipole')
ax2.set_xlabel('X (m)'); ax2.set_ylabel('Y (m)'); ax2.set_zlabel('Z (m)')
ax2.set_title('2. B-Field Vectors', fontweight='bold')
ax2.legend(fontsize=8)

# ── Plot 3: cost curve ────────────────────────────────────────────────────
ax3 = fig.add_subplot(2, 2, 3)
iters = np.arange(1, len(costs)+1)
ax3.semilogy(iters, costs, 'o-', color='crimson', linewidth=2, markersize=5)
ax3.set_xlabel('Iteration')
ax3.set_ylabel('Cost  F(x)  [log scale]')
ax3.set_title('3. LM Convergence', fontweight='bold')
ax3.grid(True, which='both', alpha=0.3)
ax3.fill_between(iters, costs, alpha=0.1, color='crimson')

# ── Plot 4: true vs estimated ─────────────────────────────────────────────
ax4 = fig.add_subplot(2, 2, 4, projection='3d')
ax4.scatter(sensors[:,0], sensors[:,1], sensors[:,2],
            color='steelblue', s=30, alpha=0.4, label='Sensors')
scale = 0.03
ax4.scatter(*true_pos, color='crimson', s=250, marker='*', label='True dipole')
ax4.quiver(*true_pos,
           true_mom[0]*scale, true_mom[1]*scale, true_mom[2]*scale,
           color='crimson', linewidth=2.5, arrow_length_ratio=0.3)
ax4.scatter(*est_pos, color='limegreen', s=150, marker='^', label='Estimated')
ax4.quiver(*est_pos,
           est_mom[0]*scale, est_mom[1]*scale, est_mom[2]*scale,
           color='limegreen', linewidth=2.5, arrow_length_ratio=0.3)
ax4.plot([true_pos[0], est_pos[0]],
         [true_pos[1], est_pos[1]],
         [true_pos[2], est_pos[2]],
         'k--', linewidth=1.2, alpha=0.5)
err_mm = np.linalg.norm(true_pos - est_pos) * 1e3
ax4.set_xlabel('X (m)'); ax4.set_ylabel('Y (m)'); ax4.set_zlabel('Z (m)')
ax4.set_title(f'4. True vs Estimated  (Δ={err_mm:.4f} mm)', fontweight='bold')
ax4.legend(fontsize=8)

plt.tight_layout()
plt.savefig('plots/plot_dashboard.png', dpi=150, bbox_inches='tight')
plt.show()
print("Saved: plots/plot_dashboard.png")
