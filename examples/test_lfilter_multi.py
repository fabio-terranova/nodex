import time
from contextlib import contextmanager

import matplotlib.pyplot as plt
import nodex_py
import numpy as np
from scipy import signal

plot = True
n_samples = 600000
n_channels = 64
order = 5
fc = 1000
fs = 10000
btype = "low"
ftype = "butter"
spike_frequency = 50  # Hz


def spike(length: int, fs: int, amplitude: float = 1.0) -> np.ndarray:
    t = np.arange(length) / fs
    waveform = amplitude * np.exp(-((t - (length / (2 * fs))) ** 2) / (2 * (0.001**2)))
    return waveform


@contextmanager
def timed(label: str):
    t0 = time.time()
    try:
        yield
    finally:
        dt = time.time() - t0
        print(f"{label}:\t {dt:.4f} s")


b, a = signal.iirfilter(order, fc, fs=fs, btype=btype, ftype=ftype, rs=5.0, rp=5.0)

# create multichannel neural spike-like data
np.random.seed(42)
t = np.arange(n_samples) / fs
data = np.random.randn(n_channels, n_samples)
n_spikes = int(spike_frequency * (n_samples / fs))
spike_times = np.random.choice(n_samples - 20, n_spikes, replace=False) + 10
for ch in range(n_channels):
    delay = np.random.randint(-10, 11)  # delay between -10 and 10 samples
    for st in spike_times:
        if 10 <= st + delay < n_samples - 10:
            # the longer the delay the smaller the amplitude
            amplitude = 5.0 - abs(delay) / 2.0
            data[ch, st + delay - 10 : st + delay + 10] += spike(20, fs, amplitude)

with timed("Time taken (py, axis=1)"):
    output_py = signal.lfilter(b, a, data, axis=1)

with timed("Time taken (cpp, multi)"):
    output_cpp_multi = np.array(nodex_py.lfilter_multi(b=b, a=a, x=data))

if plot:
    # square grid of subplots
    n = int(np.ceil(np.sqrt(n_channels)))
    fig, axes = plt.subplots(n, n, figsize=(12, 8), sharex=True, sharey=True)
    # wspace and hspace to 0.1
    fig.subplots_adjust(wspace=0.1, hspace=0.1)
    axes = axes.flatten()
    for ch in range(n_channels):
        axes[ch].plot(t, data[ch, :], label="input", alpha=0.3)
        axes[ch].plot(t, output_py[ch, :], label="scipy", alpha=0.7)
        axes[ch].plot(
            t, output_cpp_multi[ch, :], label="nodex multi", linestyle="--", alpha=0.7
        )
    axes[0].legend()
    plt.show()
