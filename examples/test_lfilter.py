import time

import matplotlib.pyplot as plt
import numpy as np
import noddy_py
from scipy import signal

plot = True
n_samples = 100000
order = 2
fc = 400
fs = 10000
epsilon = 1e-9
ftype = "butter"
# ftype = "cheby1"
# ftype = "cheby2"
btype = "low"
# btype = "high"

b, a = signal.iirfilter(order, fc, fs=fs, btype=btype, ftype=ftype, rs=5.0, rp=5.0)

# load data from file
data = np.random.standard_normal(n_samples)
x = np.linspace(0, n_samples / fs, n_samples)

time_start = time.time()
output = signal.lfilter(b, a, data)
time_end = time.time()
print(f"Time taken (py):\t {time_end - time_start} s")

time_start = time.time()
outputcpp = np.array(noddy_py.lfilter(b=b, a=a, x=data))
time_end = time.time()
print(f"Time taken (cpp):\t {time_end - time_start} s")

time_start = time.time()
outputcpp_fft = np.array(noddy_py.fft_filter(b=b, a=a, x=data, epsilon=epsilon))
time_end = time.time()
print(f"Time taken (cpp, fft):\t {time_end - time_start} s")

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
    plt.subplot(2, 1, 1)
    plt.plot(x, data, label="data")
    plt.plot(x, output, label="filtered (py)")
    plt.plot(x, outputcpp, label="filtered (cpp)")
    plt.plot(x, outputcpp_fft, label="filtered (cpp, fft)")
    plt.ylabel("Amplitude")
    plt.legend()

    plt.subplot(2, 1, 2, sharex=plt.gca())
    plt.plot(x, output - outputcpp, label="filtered (cpp)")
    plt.ylabel("Amplitude")
    plt.plot(x, output - outputcpp_fft, label="filtered (cpp, fft)")
    plt.xlabel("Time (s)")
    plt.ylabel("Error")
    plt.legend()
    plt.tight_layout()
    plt.show()

    # plot fft (magnitude and phase)
    plt.subplot(2, 1, 1)
    plt.plot(freqs, np.abs(fft_data), label="data")
    plt.plot(freqs, np.abs(fft_output), label="filtered (py)")
    plt.plot(freqs, np.abs(fft_outputcpp), label="filtered (cpp)")
    plt.plot(freqs, np.abs(fft_outputcpp_fft), label="filtered (cpp, fft)")
    plt.ylabel("Amplitude")
    plt.semilogx()
    plt.legend()
    plt.subplot(2, 1, 2, sharex=plt.gca())
    plt.plot(freqs, np.angle(fft_data), label="data")
    plt.plot(freqs, np.angle(fft_output), label="filtered (py)")
    plt.plot(freqs, np.angle(fft_outputcpp), label="filtered (cpp)")
    plt.plot(freqs, np.angle(fft_outputcpp_fft), label="filtered (cpp, fft)")
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("Phase (radians)")
    plt.semilogx()
    plt.legend()
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
