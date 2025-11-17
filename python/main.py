import serial
import time

import threading
import queue
import pyqtgraph as pg
import realtime_fft_widget as rtimegui
import sys
import data_packet

class DataThreading:
    def __init__(self, serial_port: str, baud_rate: int) -> None:
        try:
            self._serial = serial.Serial(data_packet.SERIAL_PORT, data_packet.BAUD_RATE, timeout=0)
        except serial.SerialException as e:
            print(f"Error opening serial port {data_packet.SERIAL_PORT}: {e}")
            exit()

        self.data_handler = data_packet.DataHandler()
        self.bytes_buffer = bytearray()
        
        self._share_data_queue = queue.Queue()
        self._stop_event = threading.Event()

        self._io_thread = threading.Thread(target=self._serial_reader, daemon=True)
        self._proc_thread = threading.Thread(target=self._serial_proc, daemon=True)

        self._call_backs = []
    
    def _serial_reader(self):
        while not self._stop_event.is_set():
            try:
                in_waiting = self._serial.in_waiting # check the number of bytes of the data in buffer
                print("in_waiting", in_waiting)
                if in_waiting > 0:
                    new_bytes = self._serial.read(in_waiting) # read whole data in buffer just one time
                    self._share_data_queue.put(new_bytes) # put the data into the queue to the processor function process the bytes data 
                time.sleep(0.001) # To prevent the Busy-waiting due to 100& CPU share. Removed for faster processing.
                
            except (OSError, serial.SerialException):
                print("serial port error")
                break
        print("I/O thread is shut down.")

    def _serial_proc(self):
        while not self._stop_event.is_set():
            try:
                if not self._share_data_queue.empty():
                    new_bytes = self._share_data_queue.get()
                    self.bytes_buffer.extend(new_bytes)

                if len(self.bytes_buffer) > 0:
                    for data_valid in self.data_handler.get_vaild_data(self.bytes_buffer):
                        for callback_fn in self._call_backs:
                            callback_fn(data_valid)

            except queue.Empty:
                continue
        print("Processing thread is shut down.")

    def add_callback(self, callback):
        self._call_backs.append(callback)

    def remove_callback(self, callback):
        self._call_backs.remove(callback)
    
    def start(self):
        self._io_thread.start()
        self._proc_thread.start()

    def stop(self):
        self._stop_event.set()
        self._io_thread.join()
        self._proc_thread.join()
        self._serial.close()
        print("stop threads")


from datetime import datetime
import csv
from collections import deque

class DataLogger:
    def __init__(self, base_dir="./output/"):

        self.file_name = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.file_path = base_dir + self.file_name + ".csv"
        self.file = open(self.file_path, "w", newline='')
        self.writer = csv.writer(self.file)
        self.queue = deque(maxlen=1000)

        self._stop_event = threading.Event()
        self._log_thread = threading.Thread(target=self._write_data, daemon=True)

    def add_data(self, data):
        self.queue.appendleft(data)
    
    def _write_data(self):
        while not self._stop_event.is_set():
            try:
                packet_data = self.queue.pop()
                row = [time.time()] + list(packet_data.mic)
                self.writer.writerow(row)
            except IndexError:
                continue
    
        while len(self.queue) > 0:
            packet_data = self.queue.pop()
            print("saving lest data", packet_data)
            row = [time.time()] + list(packet_data.mic)
            self.writer.writerow(row)

    def start(self):
        self._log_thread.start()

    def stop(self):
        print("Closing logger... waiting for log thread to finish.")
        self._stop_event.set()
        self._log_thread.join()
        self.file.close()
        print("Logger closed.")


def example_packet_processor(packet_data):
    # print(f"  [콜백 수신] Tick: {packet_data.tick}, Acc: {packet_data.acc}")
    print(time.time(), " ", packet_data)

def main():

    app = pg.mkQApp("Real Time Spectrogram")
    widget = rtimegui.RealtimeSpectrogramWidget()
    widget.show()
    widget.resize(800, 600)

    logger = DataLogger()
    logger.start()

    dataThreading = DataThreading(data_packet.SERIAL_PORT, data_packet.BAUD_RATE)
    # dataThreading.add_callback(example_packet_processor)
    dataThreading.add_callback(logger.add_data)
    dataThreading.add_callback(widget.run)

    dataThreading.start()

    sys.exit(app.exec())
    # time.sleep(10)
    dataThreading.stop()
    logger.stop()


if __name__ == "__main__":
    main()