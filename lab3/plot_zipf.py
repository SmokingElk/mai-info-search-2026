import csv
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ranks = []
freqs = []
with open("term_freq.csv", newline="", encoding="utf-8", errors="replace") as f:
    reader = csv.DictReader(f)
    for row in reader:
        ranks.append(int(row["rank"]))
        freqs.append(int(row["frequency"]))

r = np.array(ranks, dtype=float)
f = np.array(freqs, dtype=float)

# Аппроксимация законом Ципфа f = c / r^a методом наименьших квадратов
# в логарифмической шкале: ln f = ln c - a * ln r
mask = f > 0
lr = np.log(r[mask])
lf = np.log(f[mask])
coeffs, residuals, _, _, _ = np.polyfit(lr, lf, 1, full=True)
slope, intercept = coeffs
a = -slope
c = np.exp(intercept)

ss_tot = np.sum((lf - lf.mean()) ** 2)
r2 = 1.0 - residuals[0] / ss_tot if len(residuals) else float("nan")

# Статистика для объяснения расхождений
hapaxes = int(np.sum(f == 1))
top10_share = f[:10].sum() / f.sum()
top100_share = f[:100].sum() / f.sum()

print(f"Терминов в словаре: {len(r)}")
print(f"Всего вхождений: {int(f.sum())}")
print(f"Закон Ципфа: f = c / r^a,  c = {c:.1f},  a = {a:.3f},  R^2 = {r2:.4f}")
print(f"Гапаксы (freq=1): {hapaxes} ({100 * hapaxes / len(r):.1f}% словаря)")
print(f"Доля топ-10 терминов: {top10_share * 100:.1f}% всех вхождений")
print(f"Доля топ-100 терминов: {top100_share * 100:.1f}% всех вхождений")

fig, ax = plt.subplots(figsize=(7.0, 4.2))
ax.loglog(r, f, ".", markersize=1.5, color="#1f77b4",
          label="Частоты терминов корпуса", rasterized=True)
zipf_curve = c / r ** a
ax.loglog(r, zipf_curve, "-", linewidth=1.8, color="#d62728",
          label=f"Закон Ципфа $f = c\\, /\\, r^{{a}}$, $c \\approx {c:.0f}$, $a \\approx {a:.2f}$")
ax.set_xlabel("Ранг термина r")
ax.set_ylabel("Частота термина f")
ax.grid(True, which="both", linestyle=":", linewidth=0.5, alpha=0.6)
ax.legend(loc="upper right", fontsize=9)
fig.tight_layout()
fig.savefig("zipf.pdf")
print("График сохранён в zipf.pdf")
