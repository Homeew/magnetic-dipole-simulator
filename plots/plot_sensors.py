import os
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# run the C++ binary and capture CSV output
result = subprocess.run(
    ["./build/dipole_sim", "--csv"],
    capture_output=True, text=True,
    cwd=os.path.dirname(os.path.abspath(__file__)) or "."
)
lines = result.stdout.strip().split("\n")

# parse the CSV sections
sensors = []
true_pos = None
section = None

for line in lines:
    if line == "SENSORS":
        section = "sensors"
    elif line == "TRUE":
        section = "true"
    elif "," in line:
        vals = list(map(float, line.split(",")))
        if section == "sensors":
            sensors.append(vals)
        elif section == "true" and true_pos is None:
            true_pos = vals

sensors  = np.array(sensors)
true_pos = np.array(true_pos)

# plot
fig = plt.figure(figsize=(8, 6))
ax  = fig.add_subplot(111, projection='3d')

# sensor positions
ax.scatter(sensors[:,0], sensors[:,1], sensors[:,2],
           color='steelblue', s=60, zorder=5, label='Sensors')

# vertical depth lines for each sensor
for s in sensors:
    ax.plot([s[0], s[0]], [s[1], s[1]], [0, s[2]],
            color='steelblue', alpha=0.2, linewidth=0.8)

# true dipole
ax.scatter(*true_pos, color='crimson', s=200, marker='*',
           zorder=10, label='True dipole')

ax.set_xlabel('X (m)')
ax.set_ylabel('Y (m)')
ax.set_zlabel('Z (m)')
ax.set_title('Sensor Array Geometry', fontsize=13, fontweight='bold')
ax.legend()
plt.tight_layout()
plt.savefig('plot1_sensor_geometry.png', dpi=150)
plt.show()
print("Saved: plot1_sensor_geometry.png")
