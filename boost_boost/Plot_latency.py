#!/usr/bin/env python3
# plot_latency.py
# pip install matplotlib pandas



import pandas as pd
import matplotlib.pyplot as plt
import sys

extra_name = ""

if len(sys.argv) > 1:
    extra_name = "_" + sys.argv[1]

df = pd.read_csv(f"server_timestamps{extra_name}.csv")

# RTT / 2 = приблизительная задержка в одну сторону
df["latency_us"] = df["rtt_ns"] / 1000.0 / 2.0
df["rtt_us"] = df["rtt_ns"] / 1000.0

avg = df["latency_us"].mean()
median = df["latency_us"].median()
avg_rtt = df["rtt_us"].mean()

print(f"RTT среднее:       {avg_rtt:.1f} мкс")
print(f"Задержка (RTT/2):  {avg:.1f} мкс")
print(f"Медиана (RTT/2):   {median:.1f} мкс")
print(f"Мин (RTT/2):       {df['latency_us'].min():.1f} мкс")
print(f"Макс (RTT/2):      {df['latency_us'].max():.1f} мкс")

fig, axes = plt.subplots(2, 1, figsize=(12, 8))

axes[0].plot(df["seq"], df["latency_us"], linewidth=0.5, alpha=0.7)
axes[0].axhline(y=avg, color="red", linestyle="--", label=f"Среднее = {avg:.1f} мкс")
axes[0].axhline(y=median, color="blue", linestyle="-.", label=f"Медиана = {median:.1f} мкс")
axes[0].set_xlabel("Номер сообщения")
axes[0].set_ylabel("Задержка (мкс)")
axes[0].set_title("Задержка каждого сообщения (RTT / 2)")
axes[0].legend()
axes[0].grid(True, alpha=0.3)

axes[1].hist(df["latency_us"], bins=50, color="green", alpha=0.7, edgecolor="white")
axes[1].axvline(x=avg, color="red", linestyle="--", label=f"Среднее = {avg:.1f} мкс")
axes[1].axvline(x=median, color="blue", linestyle="-.", label=f"Медиана = {median:.1f} мкс")
axes[1].set_xlabel("Задержка (мкс)")
axes[1].set_ylabel("Количество")
axes[1].set_title("Распределение задержек (RTT / 2)")
axes[1].legend()
axes[1].grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig(f"latency_plot{extra_name}.png", dpi=150)
print(f"График сохранён: latency_plot{extra_name}.png")
plt.show()
