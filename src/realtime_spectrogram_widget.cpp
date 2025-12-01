#include "realtime_spectrogram_widget.hpp"

RealtimeSpectrogramWidget::RealtimeSpectrogramWidget(QWidget *parent)
    : QWidget(parent), maxHistory(2000), freqBins(64) {

  setWindowTitle("Real-time FFT data visualization (Turbo colormap)");
  setAutoFillBackground(true);
  setPalette(QPalette(Qt::black));

  initializeTurboColormap();

  updateTimer = new QTimer(this);
  updateTimer->setInterval(33); // 약 30fps
  connect(updateTimer, &QTimer::timeout, this,
          QOverload<>::of(&RealtimeSpectrogramWidget::update));
  updateTimer->start();
}
RealtimeSpectrogramWidget::~RealtimeSpectrogramWidget() {}

void RealtimeSpectrogramWidget::appendData(const PacketStruct &packet) {
  std::vector<double> combined(freqBins);
  for (int i = 0; i < 32; ++i) {
    combined[i] = static_cast<double>(packet.mic1[i]);
    combined[i + 32] = static_cast<double>(packet.mic2[i]);
  }

  {
    std::lock_guard<std::mutex> lk(dataMutex);
    if (dataBuffer.size() >= maxHistory)
      dataBuffer.pop_front();
    dataBuffer.push_back(std::move(combined));
  }
}

void RealtimeSpectrogramWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.fillRect(rect(), Qt::black);

  std::vector<std::vector<double>> localFrames;
  {
    std::lock_guard<std::mutex> lk(dataMutex);
    localFrames.reserve(dataBuffer.size());
    for (const auto &v : dataBuffer) {
      localFrames.emplace_back(v.begin(), v.end());
    }
  }

  if (localFrames.empty())
    return;

  int w = width();
  int h = height();
  if (w <= 0 || h <= 0)
    return;

  int history = static_cast<int>(localFrames.size());

  for (int t = 0; t < history; ++t) {
    const auto &frame = localFrames[t];
    for (int f = 0; f < freqBins; ++f) {
      double val = std::log10(frame[f] + 1.0);
      double norm = std::clamp(val / 5.0, 0.0, 1.0);
      QColor color = turboColor(norm);

      int x = static_cast<int>((double)t / history * w);
      int y = static_cast<int>((double)f / freqBins * h);
      int nextX = static_cast<int>((double)(t + 1) / history * w);
      int nextY = static_cast<int>((double)(f + 1) / freqBins * h);

      p.fillRect(QRect(x, h - nextY, nextX - x, nextY - y), color);
    }
  }
}
