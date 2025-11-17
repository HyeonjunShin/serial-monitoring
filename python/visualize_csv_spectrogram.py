"""
visualize_csv_spectrogram.py

DataLogger에 의해 저장된 CSV 파일의 마이크 데이터를 읽어
PyQtGraph를 사용하여 스펙트로그램으로 시각화하는 스크립트입니다.

실행 전, 필요한 라이브러리를 설치해야 합니다:
  pip install pyqtgraph numpy scipy pandas
  pip install pyqt6  # 또는 pyside6, pyqt5, pyside2 등
"""
import os
import sys
import numpy as np
import pandas as pd
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets
from scipy import signal


def visualize_spectrogram_from_csv(file_path):
    """
    CSV 파일을 읽어 스펙트로그램을 생성하고 화면에 표시합니다.
    이 함수는 CSV의 데이터가 이미 FFT 변환된 주파수 데이터라고 가정합니다.

    Args:
        file_path (str): 데이터가 저장된 CSV 파일의 경로.
    """
    print(f"'{file_path}' 파일에서 FFT 변환된 데이터를 로딩합니다...")

    # --- 1. 데이터 로딩 ---
    try:
        df = pd.read_csv(file_path, header=None)
    except FileNotFoundError:
        print(f"[에러] 파일을 찾을 수 없습니다: {file_path}")
        return
    except Exception as e:
        print(f"[에러] 파일을 읽는 중 오류가 발생했습니다: {e}")
        return

    # 첫 번째 열은 타임스탬프, 나머지는 이미 FFT된 마이크 데이터입니다.
    timestamps = df.iloc[:, 0]
    mic_data_rows = df.iloc[:, 1:].to_numpy()

    # --- 2. 시간 축 및 주파수 축 정보 설정 ---
    total_duration = timestamps.iloc[-1] - timestamps.iloc[0]
    time_axis = np.linspace(0, total_duration, num=len(mic_data_rows))

    # Y축 (주파수): 32개 채널이 20000Hz를 의미 (채널당 625Hz)
    num_channels = mic_data_rows.shape[1]
    max_frequency = num_channels * 625

    print(f"데이터 시간: {total_duration:.2f}초, 주파수 범위: 0-{max_frequency}Hz")

    # 로그 스케일로 변환하여 동적 범위를 넓힙니다.
    # 데이터에 0 또는 음수가 있을 수 있으므로, 최소값을 찾아 양수로 만들어준 뒤 log 적용
    min_val = mic_data_rows.min()
    log_data = np.log10(mic_data_rows - min_val + 1e-12)

    # --- 3. PyQtGraph로 시각화 ---
    app = pg.mkQApp(f"Data Viewer: {os.path.basename(file_path)}")
    win = pg.GraphicsLayoutWidget(show=True, title=f"FFT Data Viewer: {os.path.basename(file_path)}", size=(800, 600))
    win.resize(800, 600)

    p1 = win.addPlot(title="Pre-calculated FFT Data (Frequency vs. Time)")
    p1.setLabel("left", "Frequency", units="Hz")
    p1.setLabel("bottom", "Time", units="s")

    img = pg.ImageItem()
    p1.addItem(img)
    # 데이터를 (주파수, 시간) 축으로 표시하기 위해 전치(.T)
    img.setImage(log_data.T, autoLevels=True)

    # 이미지의 좌표계를 실제 시간과 주파수 값으로 설정합니다.
    img.setRect(time_axis[0], 0, time_axis[-1] - time_axis[0], max_frequency)

    # 컬러바 추가
    bar = pg.ColorBarItem(values=(log_data.min(), log_data.max()), colorMap='viridis') 
    bar.setImageItem(img)

    sys.exit(app.exec())


if __name__ == "__main__":
    # 시각화할 CSV 파일 경로를 지정합니다.
    file_path = "output/20251031_135208.csv"
    visualize_spectrogram_from_csv(file_path)