import threading
import queue
from collections import deque
import time
import serial
import time
from datetime import datetime
import csv
import sys

import vacgrip
import pyqtgraph as pg
import realtime_fft_widget as rtimegui


class ThreadingVacGrip:
    def __init__(self, port_name, baud_rate=vacgrip.BAUD_RATE):
        self._serial = serial.Serial(port_name, baud_rate, timeout=0)
        self._packet_handler = vacgrip.VacGripPacketHandler()

        self._share_data_queue = queue.Queue()
        self._stop_event = threading.Event()

        self._packet_callbacks = []

        self._io_thread = threading.Thread(target=self._serial_reader, daemon=True)
        self._processing_thread = threading.Thread(target=self._packet_processor, daemon=True)

    def _serial_reader(self):
        while not self._stop_event.is_set():
            try:
                in_waiting = self._serial.in_waiting # check the number of bytes of the data in buffer
                if in_waiting > 0:
                    new_bytes = self._serial.read(in_waiting) # read whole data in buffer just one time
                    self._share_data_queue.put(new_bytes) # put the data into the queue to the processor function process the bytes data 
                time.sleep(0.0005) # To prevent the Busy-waiting due to 100& CPU share. Removed for faster processing.
                
            except (OSError, serial.SerialException):
                print("serial port error")
                break
        print("I/O thread is shut down.")

    def _packet_processor(self):
        input_buf = bytearray()
        while not self._stop_event.is_set():
            try:
                new_bytes = self._share_data_queue.get_nowait() # Getting data from the queue without blocking.
                input_buf += new_bytes
                print(new_bytes)

                for _, packet_data in self._packet_handler.generate_valid_packet(input_buf): # Data parsing 
                    if len(self._packet_callbacks) > 0:
                        try:
                            for callback in self._packet_callbacks:
                                callback(packet_data)
                        except Exception as e:
                            print(e)

            except queue.Empty:
                continue
        print("processing thread is shut down.")
    
    def add_callback(self, callback):
        self._packet_callbacks.append(callback)

    def start(self):
        self._serial.reset_input_buffer()
        self._stop_event.clear()
        
        self._io_thread.start()
        self._processing_thread.start()

    def stop(self):
        self._stop_event.set()
        
        self._io_thread.join()
        self._processing_thread.join()
        
        self._serial.close()
        print("stop the handler")

class DataLogger:
    def __init__(self, base_dir="./output/"):
        self._stop_event = threading.Event()

        self.file_name = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.file_path = base_dir + self.file_name + ".csv"
        self.file = open(self.file_path, "w", newline='')
        self.writer = csv.writer(self.file)
        self.queue = deque(maxlen=1000)
        self._log_thread = threading.Thread(target=self._process_queue, daemon=True)

    def callback_fn(self, packet_data: vacgrip.VacGripPacketData):
        self.queue.appendleft(packet_data)
    
    def _process_queue(self):
        while not self._stop_event.is_set():
            try:
                packet_data = self.queue.pop()
                row = [time.time()] + list(packet_data.mic)
                self.writer.writerows(row)
            except IndexError:
                continue
    
        while len(self.queue) > 0:
            packet_data = self.queue.pop()
            row = [time.time()] + list(packet_data.mic)
            self.writer.writerows(row)

    def close(self):
        print("Closing logger... waiting for log thread to finish.")
        self._stop_event.set()
        self._log_thread.join() # 스레드가 모든 작업을 마칠 때까지 대기
        self.file.close()
        print("Logger closed.")

    def start(self):
        self._log_thread.start()

if __name__ == "__main__":
    # def example_packet_processor(packet_data: vacgrip.VacGripPacketData):
    #     print(f"  [콜백 수신] Tick: {packet_data.tick}, Acc: {packet_data.acc}")
    #     print(time.time(), " ", packet_data)

    # app = pg.mkQApp("Real Time Spectrogram")
    # widget = rtimegui.RealtimeSpectrogramWidget()
    # widget.show()
    # widget.resize(800, 600)

    SERIAL_PORT = "/dev/ttyUSB0"
    logger = DataLogger()
    logger.start()
    try:
        handler = ThreadingVacGrip(SERIAL_PORT)
        # handler.add_callback(logger.callback_fn)
        # handler.add_callback(example_packet_processor)
        # handler.add_callback(widget.run)
        handler.start()

        print("60초 동안 데이터 수신 및 처리를 실행합니다...")
        time.sleep(10)
        # sys.exit(app.exec())

    except serial.SerialException as e:
        print(f"Can not access the serial port: '{SERIAL_PORT}'")
    except Exception as e:
        print(f"Unexpected error {e}")
    finally:
        if 'handler' in locals() and handler._io_thread.is_alive():
            handler.stop()
            logger.close()
        print("--- 테스트 종료 ---")