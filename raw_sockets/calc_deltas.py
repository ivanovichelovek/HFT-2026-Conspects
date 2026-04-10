import matplotlib.pyplot as plt
import numpy as np

FILE_NAME_RAW = "recv_single_raw"
FILE_NAME_BOOST = "recv_single_boost"

def calc_deltas(FILE_NAME):
  send_time = []
  receive_time = []

  OUTGOING_POINT_THRESHOLD = float('inf')

  delays = []

  with open(f"{FILE_NAME}.txt", "r") as input_file:
    for row in input_file:
      delays.append(int(row))

  plt.figure(figsize=(10, 5))
  plt.plot(range(len(delays)), delays, marker='o', linestyle='-', markersize=3)
  plt.xlabel("Message #")
  plt.ylabel("Delay (microseconds)")
  plt.title("Message delays: send -> receive")
  plt.grid(True)
  plt.savefig(f"{FILE_NAME}.png", dpi=300)
  print(f"Graph successfully saved in {FILE_NAME}.png")
  print("Average delta is", sum(delays) / len(delays), "microseconds")

  return delays


delays_raw = calc_deltas(FILE_NAME_RAW)
delays_boost = calc_deltas(FILE_NAME_BOOST)

x = np.array(delays_raw)
y = np.array(delays_boost)

print("mean raw =", x.mean())
print("mean boost =", y.mean())
print("diff =", x.mean() - y.mean())

median_raw = sorted(delays_raw)[len(delays_raw) // 2]
median_boost = sorted(delays_boost)[len(delays_boost) // 2]
print("median raw =", median_raw, "microseconds")
print("median boost =", median_boost, "microseconds")
print("median diff =", abs(median_boost - median_raw), "microseconds")