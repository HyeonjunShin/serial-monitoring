"""
pyqtgraph_spectrogram_example.py

`PyQtGraph`를 사용하여 마이크 데이터로부터 실시간 스펙트로그램을 그리는 예제입니다.

이 코드는 `vacgrip_threaded.py`의 `RealTimeVacGrip` 클래스를 사용하여 백그라운드에서
안정적으로 데이터를 수신하고, 메인 GUI 스레드에서 스펙트로그램을 업데이트합니다.

실행 전, 필요한 라이브러리를 설치해야 합니다:
  pip install pyqtgraph numpy scipy pyserial
  pip install pyqt6  # 또는 pyside6, pyqt5, pyside2 등
"""

import sys
import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets
from scipy import signal
from collections import deque
import time

# 이전에 작성한 스레드 기반의 데이터 수신 모듈
try:
    import vacgrip_threaded
    import vacgrip
    HARDWARE_AVAILABLE = True
except (ImportError, ModuleNotFoundError):
    print("[경고] vacgrip 모듈을 찾을 수 없습니다. 가상 데이터 모드로 실행됩니다.")
    HARDWARE_AVAILABLE = False
except Exception as e:
    print(f"[경고] 모듈 임포트 중 에러: {e}. 가상 데이터 모드로 실행됩니다.")
    HARDWARE_AVAILABLE = False

# --- 상수 정의 ---
# MCU의 패킷 전송률(약 990Hz)과 패킷 당 마이크 샘플 수(32)를 기반으로 계산
# 실제 환경에 맞게 조절이 필요할 수 있습니다.
MIC_SAMPLING_RATE = 990 * 32  # 마이크 샘플링 주파수 (Hz)

# 스펙트로그램을 계산할 데이터의 길이 (샘플 수)
# 약 0.5초 분량의 데이터를 사용하여 스펙트로그램을 계산합니다.
SPECTROGRAM_SAMPLES = int(MIC_SAMPLING_RATE * 0.5)

# 실시간으로 표시할 오디오 데이터의 총 길이 (슬라이딩 윈도우 크기)
# 스펙트로그램 계산에 필요한 데이터보다 약간 길게 설정합니다.
AUDIO_BUFFER_SAMPLES = int(MIC_SAMPLING_RATE * 0.6)


class SpectrogramWidget(pg.GraphicsLayoutWidget):
    """
    실시간 스펙트로그램을 표시하는 PyQtGraph 위젯.
    """
    # [스레딩] 워커 스레드(데이터 수신)에서 메인 스레드(GUI 업데이트)로 
    # 안전하게 신호를 보내기 위한 PyQt의 Signal 객체.
    # 데이터 수신 콜백에서 직접 GUI를 업데이트하면 프로그램이 충돌할 수 있습니다.
    data_ready_signal = QtCore.Signal()

    def __init__(self, serial_port, parent=None):
        super().__init__(parent)
        self.setWindowTitle("실시간 마이크 스펙트로그램")

        # --- 데이터 버퍼 및 하드웨어 핸들러 초기화 ---
        # 최근 오디오 데이터를 저장하는 양방향 큐(deque). 오래된 데이터는 자동으로 버려집니다.
        self.audio_buffer = deque(maxlen=AUDIO_BUFFER_SAMPLES)
        self.serial_port = serial_port
        self.handler = None
        self.mock_data_timer = None

        # --- PyQtGraph UI 설정 ---
        # 플롯 아이템 추가
        self.plot_item = self.addPlot(title="Spectrogram")
        self.plot_item.setLabel('left', 'Frequency', units='Hz')
        self.plot_item.setLabel('bottom', 'Time', units='s')

        # 스펙트로그램을 표시할 이미지 아이템 생성
        self.image_item = pg.ImageItem()
        self.plot_item.addItem(self.image_item)

        # 컬러맵 설정 (viridis가 보기 좋음)
        colormap = pg.colormap.get('viridis')
        bar = pg.ColorBarItem(values=(0, 1), colorMap=colormap)
        bar.setImageItem(self.image_item)

        # --- 시그널과 슬롯 연결 ---
        # 데이터가 준비되었다는 신호가 오면, `_update_plot` 함수를 실행하여 화면을 업데이트합니다.
        self.data_ready_signal.connect(self._update_plot)

    def start(self):
        """데이터 수신 및 처리를 시작합니다."""
        if HARDWARE_AVAILABLE and self.serial_port:
            try:
                # RealTimeVacGrip 핸들러를 생성하고, 데이터 수신 시 호출될 콜백을 지정합니다.
                self.handler = vacgrip_threaded.ThreadingVacGrip(
                    self.serial_port, 
                    packet_callback=self._data_received_callback
                )
                self.handler.start()
                print(f"시리얼 포트({self.serial_port})에서 데이터 수신을 시작합니다.")
                return
            except Exception as e:
                print(f"[에러] 하드웨어 시작 실패: {e}. 가상 데이터 모드로 전환합니다.")
        
        # 하드웨어를 사용할 수 없거나 실패한 경우, 가상 데이터 타이머를 시작합니다.
        self._start_mock_data_mode()

    def _start_mock_data_mode(self):
        """가상 데이터 생성 타이머를 시작합니다."""
        print("가상 데이터 모드를 시작합니다. (10ms 마다 데이터 생성)")
        self.mock_data_timer = QtCore.QTimer()
        self.mock_data_timer.timeout.connect(self._generate_mock_data)
        self.mock_data_timer.start(10) # 10ms 마다 `_generate_mock_data` 호출

    def _generate_mock_data(self):
        """가상 오디오 데이터(Sine wave)를 생성하고 콜백을 직접 호출합니다."""
        # 패킷당 32개의 샘플이 있는 것처럼 시뮬레이션
        num_samples = 32
        t = time.time()
        # 시간에 따라 주파수가 변하는 Sine wave 생성
        freq = 1000 + 800 * np.sin(t * 2)
        t_axis = np.arange(num_samples) / MIC_SAMPLING_RATE
        mock_mic_data = (np.sin(2 * np.pi * freq * t_axis) * 2048).astype(np.uint16)
        
        # 실제 데이터인 것처럼 콜백 함수를 호출합니다.
        # 실제 `VacGripPacketData` 객체 대신, `mic` 속성만 있는 간단한 객체를 사용합니다.
        class MockPacket: pass
        mock_packet = MockPacket()
        mock_packet.mic = tuple(mock_mic_data)
        self._data_received_callback(mock_packet)

    def _data_received_callback(self, packet_data):
        """
        [워커 스레드에서 실행] RealTimeVacGrip 핸들러가 호출하는 콜백 함수.
        수신된 마이크 데이터를 버퍼에 추가하고, GUI 업데이트를 위해 시그널을 보냅니다.
        """
        # 패킷에서 마이크 데이터를 가져와 버퍼의 오른쪽에 추가합니다.
        self.audio_buffer.extend(packet_data.mic)
        # 메인 GUI 스레드에 화면을 업데이트하라고 신호를 보냅니다.
        self.data_ready_signal.emit()

    def _update_plot(self):
        """
        [메인 GUI 스레드에서 실행] 스펙트로그램을 계산하고 화면을 업데이트합니다.
        """
        # 버퍼에 스펙트로그램을 계산할 만큼 충분한 데이터가 쌓였는지 확인합니다.
        if len(self.audio_buffer) < SPECTROGRAM_SAMPLES:
            return

        # 버퍼에서 스펙트로그램 계산에 사용할 데이터만 복사합니다.
        data_to_process = np.array(list(self.audio_buffer))[-SPECTROGRAM_SAMPLES:]
        
        # SciPy를 사용하여 스펙트로그램을 계산합니다.
        f, t, Sxx = signal.spectrogram(
            data_to_process, 
            fs=MIC_SAMPLING_RATE, 
            nperseg=512,        # 각 세그먼트의 길이
            noverlap=256        # 세그먼트 간의 겹침
        )

        # PyQtGraph의 ImageItem에 맞게 데이터를 변환하고 업데이트합니다.
        # 로그 스케일로 변환하여 동적 범위를 넓힙니다.
        Sxx_log = np.log10(Sxx + 1e-12) # 0이 되는 것을 방지하기 위해 작은 값 추가
        
        # 이미지 아이템을 새로운 스펙트로그램 데이터로 업데이트합니다.
        self.image_item.setImage(Sxx_log.T, autoLevels=True)
        # 이미지의 위치와 크기를 시간과 주파수 축에 맞게 설정합니다.
        self.image_item.setRect(t[0], f[0], t[-1] - t[0], f[-1] - f[0])

    def closeEvent(self, event):
        """윈도우가 닫힐 때 호출되는 함수. 스레드를 안전하게 종료합니다."""
        print("윈도우 닫힘. 리소스를 정리합니다.")
        if self.handler:
            self.handler.stop()
        if self.mock_data_timer:
            self.mock_data_timer.stop()
        event.accept()


if __name__ == '__main__':
    # PyQtGraph 애플리케이션 생성
    app = pg.mkQApp("Spectrogram Example")

    # --- 실제 포트 설정 ---
    # 사용자의 실제 시리얼 포트 이름을 여기에 입력하세요.
    # 포트가 없거나 잘못된 경우, 가상 데이터 모드로 자동 전환됩니다.
    SERIAL_PORT = "/dev/ttyUSB0"  # <-- Linux 예시. Windows는 "COM3" 등

    # 위젯 인스턴스 생성
    win = SpectrogramWidget(serial_port=SERIAL_PORT)
    win.show()
    win.resize(800, 600)
    
    # 데이터 수신 시작
    win.start()

    # Qt 이벤트 루프 실행
    sys.exit(app.exec())
