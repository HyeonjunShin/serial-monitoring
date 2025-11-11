#include "device.hpp"
#include "realtime_spectrogram_widget.hpp"
#include <QApplication>
#include <QDateTime>
#include <QTimer>
#include <random>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  RealtimeSpectrogramWidget widget;
  widget.resize(1000, 600);
  widget.show();

  // 랜덤 노이즈 기반 샘플 데이터 생성기
  QTimer generator;
  QObject::connect(&generator, &QTimer::timeout, [&]() {
    PacketStruct packet{};
    packet.header[0] = 0xAA;
    packet.data_size = sizeof(PacketStruct);
    packet.tick = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());
    packet.flag = true;

    static double phase = 0.0;
    for (int i = 0; i < 32; ++i) {
      double t = phase + i * 0.3;
      // sin 파형 + 약간의 랜덤 노이즈
      packet.mic1[i] =
          static_cast<uint16_t>(2048 + 1800 * std::sin(t) + (rand() % 200));
      packet.mic2[i] = static_cast<uint16_t>(2048 + 1800 * std::cos(t * 0.9) +
                                             (rand() % 200));
    }
    phase += 0.15;

    widget.appendData(packet);
  });

  generator.start(1); // 33ms 간격으로 데이터 발생 (~30fps)

  return app.exec();
}
