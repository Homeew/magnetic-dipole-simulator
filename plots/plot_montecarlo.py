import subprocess, sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ── Run simulator ────────────────────────────────────────────────────────────
print("Running Monte Carlo (300 trials × 6 noise levels)…")
raw = subprocess.run(["../build/dipole_sim", "--montecarlo"],
                     capture_output=True, text=True, cwd=__file__.rsplit("/",1)[0])
lines = raw.stdout.strip().split("\n")

# ── Parse CSV ────────────────────────────────────────────────────────────────
start = lines.index("MONTECARLO") + 2
sigmas, pos_errs, mom_errs, iters, convd = [], [], [], [], []
for ln in lines[start:]:
    if not ln.strip(): continue
    s, t, c, pe, me, it = ln.split(",")
    sigmas.append(float(s)); pos_errs.append(float(pe))
    mom_errs.append(float(me)); iters.append(int(it)); convd.append(int(c))

sigmas    = np.array(sigmas)
pos_errs  = np.array(pos_errs)
mom_errs  = np.array(mom_errs)
iters     = np.array(iters)
convd     = np.array(convd)

unique_s  = sorted(set(sigmas))

# ── Figure ───────────────────────────────────────────────────────────────────
fig, axes = plt.subplots(2, 2, figsize=(12, 9))
fig.suptitle("Monte Carlo Analysis — LM Dipole Localisation\n"
             "300 trials per noise level, ring array (16 sensors)", fontsize=13)

# ── 1. Box plot: position error vs noise level ───────────────────────────────
ax = axes[0, 0]
data_box = [pos_errs[sigmas == s] for s in unique_s]
bp = ax.boxplot(data_box, patch_artist=True, showfliers=True)
colours = plt.cm.plasma(np.linspace(0.1, 0.85, len(unique_s)))
for patch, col in zip(bp["boxes"], colours):
    patch.set_facecolor(col); patch.set_alpha(0.7)
ax.set_yscale("log")
ax.set_xticklabels([f"{s:.0e}" for s in unique_s], rotation=30, fontsize=8)
ax.set_xlabel("Noise σ (T)"); ax.set_ylabel("Position error (mm)")
ax.set_title("Position Error Distribution")
ax.grid(True, which="both", ls="--", alpha=0.4)

# ── 2. Mean ± std vs noise level (log-log) ───────────────────────────────────
ax = axes[0, 1]
means = [pos_errs[sigmas == s].mean() for s in unique_s]
stds  = [pos_errs[sigmas == s].std()  for s in unique_s]
ax.errorbar(unique_s, means, yerr=stds, fmt="o-", capsize=5,
            color="steelblue", linewidth=2, markersize=6)
ax.set_xscale("log"); ax.set_yscale("log")
ax.set_xlabel("Noise σ (T)"); ax.set_ylabel("Mean position error (mm)")
ax.set_title("Mean Error vs Noise Level")
ax.grid(True, which="both", ls="--", alpha=0.4)

# ── 3. Histogram at σ = 1 nT ─────────────────────────────────────────────────
ax = axes[1, 0]
target_sigma = 1e-9
mask = sigmas == target_sigma
pe_1nT = pos_errs[mask]
ax.hist(pe_1nT, bins=30, color="mediumseagreen", edgecolor="white", alpha=0.85)
ax.axvline(np.median(pe_1nT), color="red", ls="--", lw=1.5,
           label=f"Median: {np.median(pe_1nT):.4f} mm")
ax.axvline(np.percentile(pe_1nT, 95), color="orange", ls="--", lw=1.5,
           label=f"95th pct: {np.percentile(pe_1nT, 95):.4f} mm")
ax.set_xlabel("Position error (mm)"); ax.set_ylabel("Count")
ax.set_title(f"Error Histogram at σ = 1 nT")
ax.legend(fontsize=8); ax.grid(True, ls="--", alpha=0.4)

# ── 4. Convergence rate vs noise level ───────────────────────────────────────
ax = axes[1, 1]
conv_rates = [convd[sigmas == s].mean() * 100 for s in unique_s]
mean_iters = [iters[sigmas == s].mean() for s in unique_s]
ax2 = ax.twinx()
ax.bar(range(len(unique_s)), conv_rates, color="cornflowerblue", alpha=0.7,
       label="Conv. rate (%)")
ax2.plot(range(len(unique_s)), mean_iters, "ro-", lw=2, ms=6,
         label="Mean iters")
ax.set_xticks(range(len(unique_s)))
ax.set_xticklabels([f"{s:.0e}" for s in unique_s], rotation=30, fontsize=8)
ax.set_xlabel("Noise σ (T)"); ax.set_ylabel("Convergence rate (%)")
ax2.set_ylabel("Mean iterations")
ax.set_title("Convergence Rate & Speed")
ax.set_ylim(0, 110)
lines1, labels1 = ax.get_legend_handles_labels()
lines2, labels2 = ax2.get_legend_handles_labels()
ax.legend(lines1 + lines2, labels1 + labels2, fontsize=8)
ax.grid(True, ls="--", alpha=0.4)

plt.tight_layout()
out = "montecarlo_dashboard.png"
plt.savefig(out, dpi=150, bbox_inches="tight")
print(f"Saved {out}")
plt.show()
