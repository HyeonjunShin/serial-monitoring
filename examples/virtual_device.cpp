#include "device.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

int main() {
  // 소켓 생성
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    return 1;
  }

  // 서버 주소 설정
  sockaddr_in serv_addr{};
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(9000);                     // 포트 번호
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr); // 서버 IP

  // 서버에 연결
  if (connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("connect");
    close(sock);
    return 1;
  }

  // 패킷 생성 및 초기화
  PacketStruct pkt{};
  std::memcpy(pkt.header, HEADER, HEADER_SIZE);
  pkt.data_size = sizeof(pkt) - HEADER_SIZE - sizeof(pkt.crc);
  pkt.tick = 123456;
  pkt.flag = true;
  for (int i = 0; i < 32; ++i) {
    pkt.mic1[i] = i;
    pkt.mic2[i] = 100 + i;
  }

  // CRC 계산
  pkt.crc = calcCRC(reinterpret_cast<const uint8_t *>(&pkt),
                    sizeof(pkt) - sizeof(pkt.crc));

  // 전송
  ssize_t sent = send(sock, &pkt, sizeof(pkt), 0);
  if (sent != sizeof(pkt)) {
    std::cerr << "Send error: " << sent << std::endl;
  } else {
    std::cout << "Packet sent (" << sent << " bytes)" << std::endl;
  }

  close(sock);
  return 0;
}

// socat -d -d pty,raw,echo=0,link=/tmp/virtual_serial tcp-listen:9000,reuseaddr