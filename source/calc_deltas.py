import matplotlib.pyplot as plt
import numpy as np
from scipy import stats

FILE_NAME_RAW = "recv_duo_raw"
FILE_NAME_BOOST = "recv_duo_boost"

def calc_deltas(FILE_NAME):
  send_time = []
  receive_time = []

  OUTGOING_POINT_THRESHOLD = float('inf')

  delays = []

  with open(f"{FILE_NAME}.txt", "r") as input_file:
    for row in input_file:
      if int(row) < 100_000:
        delays.append(int(row))

  # with open(f"{FILE_NAME}.txt", "r") as input_file:
  #   for line in input_file:
  #     send, receive = map(int, line.split())
  #     if send > 0 and receive > 0:
  #       send_time.append(send)
  #       receive_time.append(receive)

  # delays = []

  # for i in range(len(send_time)):
  #   delta = receive_time[i] - send_time[i]
  #   if delta < OUTGOING_POINT_THRESHOLD:
  #     delays.append(receive_time[i] - send_time[i])

  # delays.sort()
  # print(delays[:100])

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