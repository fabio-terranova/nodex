import time
from contextlib import contextmanager

import matplotlib.pyplot as plt
import nodex_py
import numpy as np
from scipy import signal

plot = True
n_samples = 100000
order = 5
fc = 400
fs = 10000
epsilon = 1e-6
ftype = "butter"
# ftype = "cheby1"
# ftype = "cheby2"
btype = "low"
# btype = "high"


@contextmanager
def timed(label: str):
    t0 = time.time()
    yield
    dt = time.time() - t0
    print(f"{label}:\t {dt} s")


b, a = signal.iirfilter(order, fc, fs=fs, btype=btype, ftype=ftype, rs=5.0, rp=5.0)

# load data from file
data = np.random.standard_normal(n_samples)
x = np.linspace(0, n_samples / fs, n_samples)

with timed("Time taken (py)"):
    output = signal.lfilter(b, a, data)

with timed("Time taken (cpp)"):
    outputcpp = np.array(nodex_py.lfilter(b=b, a=a, x=data))

with timed("Time taken (cpp, fft)"):
    outputcpp_fft = np.array(nodex_py.fft_filter(b=b, a=a, x=data, epsilon=epsilon))

# fft
fft_data = np.fft.rfft(data)
fft_output = np.fft.rfft(output)
fft_outputcpp = np.fft.rfft(outputcpp)
fft_outputcpp_fft = np.fft.rfft(outputcpp_fft)
freqs = np.fft.rfftfreq(len(data), 1 / fs)

# psd
f, Pxx_den = signal.welch(data, fs)
fpy, Pxx_denpy = signal.welch(output, fs)
fcpp, Pxx_dencpp = signal.welch(outputcpp, fs)
fcpp_fft, Pxx_dencpp_fft = signal.welch(outputcpp_fft, fs)

if plot:
    # share x axis
    _, (ax1, ax2) = plt.subplots(nrows=2, sharex=True)
    ax1.plot(x, data, label="data")
    ax1.plot(x, output, label="filtered (py)")
    ax1.plot(x, outputcpp, label="filtered (cpp)")
    ax1.plot(x, outputcpp_fft, label="filtered (cpp, fft)")
    ax1.set_ylabel("Amplitude")
    ax1.legend()

    ax2.plot(x, output - outputcpp, label="filtered (cpp)")
    ax2.set_ylabel("Amplitude")
    ax2.plot(x, output - outputcpp_fft, label="filtered (cpp, fft)")
    ax2.set_xlabel("Time (s)")
    ax2.set_ylabel("Error")
    ax2.legend()

    plt.tight_layout()
    plt.show()

    # plot fft (magnitude and phase)
    _, (ax1, ax2) = plt.subplots(nrows=2, sharex=True)
    ax1.plot(freqs, np.abs(fft_data), label="data")
    ax1.plot(freqs, np.abs(fft_output), label="filtered (py)")
    ax1.plot(freqs, np.abs(fft_outputcpp), label="filtered (cpp)")
    ax1.plot(freqs, np.abs(fft_outputcpp_fft), label="filtered (cpp, fft)")
    ax1.set_ylabel("Amplitude")
    ax1.legend()
    ax2.plot(freqs, np.angle(fft_data), label="data")
    ax2.plot(freqs, np.angle(fft_output), label="filtered (py)")
    ax2.plot(freqs, np.angle(fft_outputcpp), label="filtered (cpp)")
    ax2.plot(freqs, np.angle(fft_outputcpp_fft), label="filtered (cpp, fft)")
    ax2.set_xlabel("Frequency (Hz)")
    ax2.set_ylabel("Phase (radians)")
    ax2.legend()

    plt.semilogx()
    plt.tight_layout()
    plt.show()

    # plot psd
    plt.plot(f, 10 * np.log10(Pxx_den), label="data")
    plt.plot(fpy, 10 * np.log10(Pxx_denpy), label="filtered (py)")
    plt.plot(fcpp, 10 * np.log10(Pxx_dencpp), label="filtered (cpp)")
    plt.plot(fcpp_fft, 10 * np.log10(Pxx_dencpp_fft), label="filtered (cpp, fft)")
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("PSD (dB/Hz)")
    plt.semilogx()
    plt.legend()
    plt.tight_layout()
    plt.show()
