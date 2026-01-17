import sys
import time
import numpy as np
import pyaudio
import serial
from PyQt5.QtCore import Qt, QThread, pyqtSignal
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QProgressBar

# -------- CONFIG --------
CHUNK = 512                # Lower chunk size = lower latency
RATE = 44100
THRESHOLD_MARGIN = 2000
SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200
MIN_LOUD_FRAMES = 3
# ------------------------

class AudioWorker(QThread):
    peak_signal = pyqtSignal(int)
    avg_signal = pyqtSignal(int)
    state_signal = pyqtSignal(bool)

    def __init__(self):
        super().__init__()
        self.running = True
        self.last_loud = False
        self.peak_window = []  # for short-term average
        self.threshold = 0
        self.loud_duration_frames = 0

    def get_peak(self, data):
        samples = np.frombuffer(data, dtype=np.int16)
        return np.max(np.abs(samples))

    def run(self):
        self.audio = pyaudio.PyAudio()
        self.stream = self.audio.open(format=pyaudio.paInt16,
                                      channels=1,
                                      rate=RATE,
                                      input=True,
                                      frames_per_buffer=CHUNK)

        self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
        time.sleep(2)

        # Ambient sampling (fixed threshold)
        peaks = []
        for _ in range(int(RATE / CHUNK * 3)):
            data = self.stream.read(CHUNK, exception_on_overflow=False)
            peaks.append(self.get_peak(data))
        ambient_avg = np.mean(peaks)
        self.threshold = ambient_avg + THRESHOLD_MARGIN

        print(f"Ambient avg: {ambient_avg}, Threshold: {self.threshold}")

        while self.running:
            data = self.stream.read(CHUNK, exception_on_overflow=False)
            peak = self.get_peak(data)
            self.peak_signal.emit(peak)

            # Short-term average (over 5 frames ≈ 0.05s)
            self.peak_window.append(peak)
            if len(self.peak_window) > 5:
                self.peak_window.pop(0)

            avg_level = int(np.mean(self.peak_window))
            self.avg_signal.emit(avg_level)

            if avg_level > self.threshold:
                self.loud_duration_frames += 1
                if self.loud_duration_frames >= MIN_LOUD_FRAMES:
                    if not self.last_loud:
                        self.ser.write(b'J')
                        self.ser.flush()
                        self.last_loud = True
                    self.state_signal.emit(True)
            else:
                if self.last_loud:
                    self.ser.write(b'D')
                    self.ser.flush()
                    self.last_loud = False
                self.loud_duration_frames = 0
                self.state_signal.emit(False)

        self.stream.stop_stream()
        self.stream.close()
        self.audio.terminate()
        self.ser.close()

    def stop(self):
        self.running = False
        self.wait()

class AudioVisualizer(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Scream Detector (Fast)")
        self.resize(400, 200)

        self.label = QLabel("Calibrating...")
        self.label.setAlignment(Qt.AlignCenter)
        self.label.setStyleSheet("font-size: 18px;")

        self.amp_bar = QProgressBar()
        self.amp_bar.setMaximum(32767)
        self.amp_bar.setTextVisible(False)

        self.avg_bar = QProgressBar()
        self.avg_bar.setMaximum(32767)
        self.avg_bar.setTextVisible(True)

        layout = QVBoxLayout()
        layout.addWidget(self.label)
        layout.addWidget(QLabel("Live Amplitude"))
        layout.addWidget(self.amp_bar)
        layout.addWidget(QLabel("Average Window Level"))
        layout.addWidget(self.avg_bar)

        self.setLayout(layout)

        self.worker = AudioWorker()
        self.worker.peak_signal.connect(self.amp_bar.setValue)
        self.worker.avg_signal.connect(self.avg_bar.setValue)
        self.worker.state_signal.connect(self.update_state)
        self.worker.start()

    def update_state(self, is_loud):
        if is_loud:
            self.label.setText("SCREAMING")
            self.label.setStyleSheet("color: red; font-weight: bold; font-size: 20px;")
        else:
            self.label.setText("Quiet")
            self.label.setStyleSheet("color: green; font-size: 18px;")

    def closeEvent(self, event):
        self.worker.stop()
        event.accept()

def main():
    app = QApplication(sys.argv)
    vis = AudioVisualizer()
    vis.show()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
