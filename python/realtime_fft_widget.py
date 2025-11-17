"""
realtime_fft_widget.py

`PyQtGraph`를 사용하여 이미 FFT 변환된 데이터를 실시간으로 시각화하는 재사용 가능한 위젯입니다.

이 클래스는 외부에서 `run(data)` 메서드를 통해 데이터를 받아 내부 버퍼에 저장하고,
이를 기반으로 스펙트로그램을 그립니다.

실행 전, 필요한 라이브러리를 설치해야 합니다:
  pip install pyqtgraph numpy
  pip install pyqt6  # 또는 pyside6, pyqt5, pyside2 등
"""

import sys
import time
from collections import deque

import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets

import data_packet

# --- 상수 정의 ---
# 화면에 표시할 시간의 길이 (패킷/데이터 수). 약 2초 분량의 데이터를 표시 (990Hz 기준)
HISTORY_SECONDS = 10
# 데이터가 들어오는 속도 (Hz). 시간 축 계산에 사용됩니다.
DATA_RATE_HZ = 200
# 화면에 표시할 데이터의 총 개수 (20초 분량)
HISTORY_LENGTH = int(DATA_RATE_HZ * HISTORY_SECONDS)
# Y축의 최대 주파수 값
MAX_FREQUENCY = 32 * 625  # 32채널 * 625Hz/채널 = 20000Hz

# pyqt is composed by mkQapp, widget, plot, plot_item
class RealtimeSpectrogramWidget(pg.GraphicsLayoutWidget):
    # To send from another thread to main gui thread

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("실시간 FFT 데이터 시각화")

        self.data_buffer = deque(maxlen=2000)

        self.plot_item = self.addPlot(title="Real-time FFT Data (Frequency vs. Time)")
        self.plot_item.setLabel("left", "Frequency", units="Hz")
        self.plot_item.setLabel("bottom", "Time", units="s")

        self.image_item = pg.ImageItem()
        self.plot_item.addItem(self.image_item)

        colormap = pg.colormap.get("viridis")
        bar = pg.ColorBarItem(values=(0, 1), colorMap=colormap)
        bar.setImageItem(self.image_item)

        self.update_timer = QtCore.QTimer()
        self.update_timer.setInterval(33)  # 1000sec/20frame = 50ms
        self.update_timer.timeout.connect(self._update_plot)
        self.update_timer.start()

    def run(self, packet_data):
        data = [time.time()] + list(packet_data.mic)
        self.data_buffer.append(data)

    def _update_plot(self):
        if len(self.data_buffer) < 2:
            return

        data_array = np.array(self.data_buffer)
        fft_data = data_array[:, 1:]

        min_val = fft_data.min()
        log_data = np.log10(fft_data - min_val + 1e-12)

        self.image_item.setImage(log_data, autoLevels=True)

        self.image_item.setRect(0, 0, HISTORY_SECONDS, MAX_FREQUENCY)

    def closeEvent(self, event):
        event.accept() # When the window is closed this event happens


if __name__ == "__main__":
    app = pg.mkQApp("Real Time Spectrogram")

    # win = pg.GraphicsLayoutWidget(show=True)

    # plot_item = win.addPlot(title="My Plot")

    # x_data = [1, 2, 3, 4]
    # y_data = [1, 4, 9, 16]
    # line_plot = plot_item.plot(x_data, y_data, pen='r') 

    win = RealtimeSpectrogramWidget()
    win.show()
    win.resize(800, 600)

    # # 2. 외부 I/O 스레드를 시뮬레이션하는 QTimer 생성
    # #    (실제 프로그램에서는 이 부분이 vacgrip_threaded나 다른 데이터 수신 로직이 됩니다.)
    # timer = QtCore.QTimer()

    # def generate_and_run_mock_data():
    #     """가상 FFT 데이터를 생성하여 win.run()을 호출하는 함수"""
    #     t = time.time()
    #     # 시간에 따라 특정 주파수 빈의 값이 변하는 것을 시뮬레이션
    #     mock_fft_data = np.zeros(32, dtype=np.uint16)
    #     mock_fft_data[5] = 1024 + 1000 * np.sin(t * 4)
    #     mock_fft_data[15] = 1024 + 1000 * np.cos(t * 2)
        
    #     # 3. 외부 스레드에서 데이터가 들어올 때마다 `run()` 메서드를 호출
    #     win.run(mock_fft_data)

    # # 타이머에 콜백 함수 연결 및 시작 (10ms 마다 데이터 생성)
    # timer.timeout.connect(generate_and_run_mock_data)
    # timer.start(10)

    # # Qt 이벤트 루프 실행
    sys.exit(app.exec())