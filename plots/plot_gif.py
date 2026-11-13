import os
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.animation as animation

# ── get data ──────────────────────────────────────────────────────────────
result = subprocess.run(
    ["./build/dipole_sim", "--csv"],
    capture_output=True, text=True,
    cwd=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
)
lines = result.stdout.strip().split("\n")

sensors, history = [], []
true_pos, true_mom, est_pos = None, None, None
section = None
true_count = est_count = 0

for line in lines:
    if line in ("SENSORS","FIELDS","TRUE","COSTS","HISTORY","ESTIMATED"):
        section = line
        continue
    if "," not in line:
        continue
    vals = list(map(float, line.split(",")))
    if section == "SENSORS":  sensors.append(vals)
    elif section == "HISTORY": history.append(vals)
    elif section == "TRUE":
        if true_count == 0: true_pos = vals
        else:               true_mom = vals
        true_count += 1
    elif section == "ESTIMATED":
        if est_count == 0: est_pos = vals
        est_count += 1

sensors  = np.array(sensors)
history  = np.array(history)   # shape (N_iters, 6)
true_pos = np.array(true_pos)
true_mom = np.array(true_mom)
est_pos  = np.array(est_pos)

# prepend the initial guess as frame 0
init = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 1.0])
history = np.vstack([init, history])

# ── build animation ───────────────────────────────────────────────────────
fig = plt.figure(figsize=(9, 7))
ax  = fig.add_subplot(111, projection='3d')

scale = 0.03

def draw_frame(i):
    ax.cla()

    # sensors
    ax.scatter(sensors[:,0], sensors[:,1], sensors[:,2],
               color='steelblue', s=40, alpha=0.5, label='Sensors')

    # true dipole
    ax.scatter(*true_pos, color='crimson', s=250, marker='*', label='True dipole')
    ax.quiver(*true_pos,
              true_mom[0]*scale, true_mom[1]*scale, true_mom[2]*scale,
              color='crimson', linewidth=2.5, arrow_length_ratio=0.3)

    # solver path so far
    path = history[:i+1, :3]
    if len(path) > 1:
        ax.plot(path[:,0], path[:,1], path[:,2],
                'o--', color='gray', linewidth=1, markersize=3, alpha=0.5)

    # current estimate
    cur = history[i]
    cur_pos = cur[:3]
    cur_mom = cur[3:]
    ax.scatter(*cur_pos, color='limegreen', s=180, marker='^',
               zorder=10, label=f'Estimate (iter {i})')
    ax.quiver(*cur_pos,
              cur_mom[0]*scale, cur_mom[1]*scale, cur_mom[2]*scale,
              color='limegreen', linewidth=2.5, arrow_length_ratio=0.3)

    err_mm = np.linalg.norm(cur_pos - true_pos) * 1e3
    ax.set_title(f'LM Solver — Iteration {i}/{len(history)-1}'
                 f'\nPosition error: {err_mm:.4f} mm',
                 fontweight='bold')
    ax.set_xlabel('X (m)')
    ax.set_ylabel('Y (m)')
    ax.set_zlabel('Z (m)')
    ax.legend(fontsize=8, loc='upper left')
    ax.set_xlim(-0.1, 0.1)
    ax.set_ylim(-0.1, 0.1)
    ax.set_zlim(0.0, 0.12)

ani = animation.FuncAnimation(fig, draw_frame,
                               frames=len(history),
                               interval=600,
                               repeat=True)

gif_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        '..', 'plots', 'lm_convergence.gif')
ani.save(gif_path, writer='pillow', dpi=100)
plt.show()
print("Saved: plots/lm_convergence.gif")
