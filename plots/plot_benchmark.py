import os
import subprocess
import numpy as np
import matplotlib.pyplot as plt

result = subprocess.run(
    ["./build/dipole_sim", "--benchmark"],
    capture_output=True, text=True,
    cwd=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
)
lines = result.stdout.strip().split("\n")

noise_data, sensor_data = [], []
section = None

for line in lines:
    if line in ("NOISE_SWEEP", "SENSOR_SWEEP"):
        section = line
        continue
    if "," not in line:
        continue
    vals = list(map(float, line.split(",")))
    if section == "NOISE_SWEEP":  noise_data.append(vals)
    elif section == "SENSOR_SWEEP": sensor_data.append(vals)

noise_data  = np.array(noise_data)
sensor_data = np.array(sensor_data)

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5))
fig.suptitle('Levenberg-Marquardt Solver — Benchmark Results',
             fontsize=14, fontweight='bold')

# ── Plot 1: position error vs noise level ─────────────────────────────────
sigmas   = noise_data[:, 0]
pos_errs = noise_data[:, 1] * 1e3   # convert to mm

ax1.loglog(sigmas, pos_errs, 'o-', color='steelblue',
           linewidth=2, markersize=7)
ax1.set_xlabel('Noise σ (T)', fontsize=12)
ax1.set_ylabel('Mean position error (mm)', fontsize=12)
ax1.set_title('Position Error vs Noise Level', fontweight='bold')
ax1.grid(True, which='both', alpha=0.3)

# reference line: error ∝ sigma (ideal linear scaling)
ref = sigmas / sigmas[0] * pos_errs[0]
ax1.loglog(sigmas, ref, '--', color='gray', alpha=0.6, label='Linear reference')
ax1.legend()

# annotate best result
ax1.annotate(f'{pos_errs[0]*1e3:.4f} μm\nat σ=1 pT',
             xy=(sigmas[0], pos_errs[0]),
             xytext=(sigmas[1], pos_errs[0]*8),
             fontsize=9, color='steelblue',
             arrowprops=dict(arrowstyle='->', color='steelblue'))

# ── Plot 2: position error vs number of sensors ───────────────────────────
n_sensors = sensor_data[:, 0].astype(int)
s_errs    = sensor_data[:, 1] * 1e3   # mm

ax2.plot(n_sensors, s_errs, 'o-', color='crimson',
         linewidth=2, markersize=7)
ax2.set_xlabel('Number of sensors', fontsize=12)
ax2.set_ylabel('Mean position error (mm)', fontsize=12)
ax2.set_title('Position Error vs Number of Sensors\n(σ = 1 nT)',
              fontweight='bold')
ax2.grid(True, alpha=0.3)
ax2.set_xticks(n_sensors)

# annotate minimum
best_idx = np.argmin(s_errs)
ax2.annotate(f'  {s_errs[best_idx]*1e3:.2f} μm\n  ({n_sensors[best_idx]} sensors)',
             xy=(n_sensors[best_idx], s_errs[best_idx]),
             fontsize=9, color='crimson')

plt.tight_layout()
plt.savefig('plots/plot_benchmark.png', dpi=150, bbox_inches='tight')
plt.show()
print("Saved: plots/plot_benchmark.png")
