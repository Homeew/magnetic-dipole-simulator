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

sensors, fields, true_pos = [], [], None
section = None

for line in lines:
    if line in ("SENSORS", "FIELDS", "TRUE", "ESTIMATED"):
        section = line
    elif "," in line:
        vals = list(map(float, line.split(",")))
        if section == "SENSORS":
            sensors.append(vals)
        elif section == "FIELDS" and len(fields) < len(sensors) + 16:
            fields.append(vals)
        elif section == "TRUE" and true_pos is None:
            true_pos = vals

sensors  = np.array(sensors)
fields   = np.array(fields)
true_pos = np.array(true_pos)

# normalise arrow lengths for visibility
magnitudes = np.linalg.norm(fields, axis=1, keepdims=True)
fields_norm = fields / magnitudes.max() * 0.025

fig = plt.figure(figsize=(9, 7))
ax  = fig.add_subplot(111, projection='3d')

# draw field vectors as arrows at each sensor
ax.quiver(sensors[:,0], sensors[:,1], sensors[:,2],
          fields_norm[:,0], fields_norm[:,1], fields_norm[:,2],
          color='steelblue', linewidth=1.5, arrow_length_ratio=0.3,
          label='B-field vector')

# sensor positions
ax.scatter(sensors[:,0], sensors[:,1], sensors[:,2],
           color='steelblue', s=40, zorder=5)

# true dipole
ax.scatter(*true_pos, color='crimson', s=200, marker='*',
           zorder=10, label='True dipole')

ax.set_xlabel('X (m)')
ax.set_ylabel('Y (m)')
ax.set_zlabel('Z (m)')
ax.set_title('B-Field Vectors at Sensor Positions', fontsize=13, fontweight='bold')
ax.legend()
plt.tight_layout()
plt.savefig('plot2_field_vectors.png', dpi=150)
plt.show()
print("Saved: plot2_field_vectors.png")
