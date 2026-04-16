#include <arpa/inet.h>
#include <linux/filter.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

struct pseudo_header {
  uint32_t source_address;
  uint32_t dest_address;
  uint8_t placeholder;
  uint8_t protocol;
  uint16_t tcp_length;
};

constexpr int DATAGRAM_LEN = 4096;
constexpr int OPT_SIZE = 20;
constexpr int IPV4_HEADER_LEN = sizeof(iphdr);
constexpr int TCP_BASE_HEADER_LEN = sizeof(tcphdr);
constexpr int OUTGOING_TCP_HEADER_LEN = TCP_BASE_HEADER_LEN + OPT_SIZE;
constexpr int OUTGOING_PACKET_HEADER_LEN = IPV4_HEADER_LEN + OUTGOING_TCP_HEADER_LEN;
constexpr int INCOMING_PACKET_HEADER_LEN = IPV4_HEADER_LEN + TCP_BASE_HEADER_LEN;
constexpr int INCOMING_FLAGS_OFFSET = IPV4_HEADER_LEN + 13;
constexpr int INCOMING_PAYLOAD_OFFSET = INCOMING_PACKET_HEADER_LEN;
constexpr int MAX_OUTGOING_PAYLOAD_LEN = DATAGRAM_LEN - OUTGOING_PACKET_HEADER_LEN;
constexpr int MESSAGE_COUNT = 10000;

static_assert(sizeof(iphdr) == 20, "Unexpected IPv4 header length");
static_assert(sizeof(tcphdr) == 20, "Unexpected TCP header length");

unsigned short checksum(const char* buf, unsigned size) {
  unsigned sum = 0;
  unsigned i = 0;
  for (i = 0; i + 1 < size; i += 2) {
    const unsigned short word16 = *reinterpret_cast<const unsigned short*>(&buf[i]);
    sum += word16;
  } if (size & 1U) {
    const unsigned short word16 = static_cast<unsigned char>(buf[i]);
    sum += word16;
  }
  while (sum >> 16U) {
    sum = (sum & 0xFFFFU) + (sum >> 16U);
  }
  return static_cast<unsigned short>(~sum);
}

struct prepared_packet {
  std::array<char, DATAGRAM_LEN> datagram{};
  std::array<char, DATAGRAM_LEN + sizeof(pseudo_header)> pseudogram{};
};

iphdr* packet_ip_header(prepared_packet* packet) {
  return reinterpret_cast<iphdr*>(packet->datagram.data());
}

tcphdr* packet_tcp_header(prepared_packet* packet) {
  return reinterpret_cast<tcphdr*>(packet->datagram.data() + IPV4_HEADER_LEN);
}

char* packet_payload(prepared_packet* packet) {
  return packet->datagram.data() + OUTGOING_PACKET_HEADER_LEN;
}

pseudo_header* packet_pseudo_header(prepared_packet* packet) {
  return reinterpret_cast<pseudo_header*>(packet->pseudogram.data());
}

char* packet_pseudo_tcp_segment(prepared_packet* packet) {
  return packet->pseudogram.data() + sizeof(pseudo_header);
}

tcphdr* packet_pseudo_tcp_header(prepared_packet* packet) {
  return reinterpret_cast<tcphdr*>(packet_pseudo_tcp_segment(packet));
}

void initialize_base_packet(prepared_packet* packet, const sockaddr_in* src,
                            const sockaddr_in* dst, bool push_flag) {
  packet->datagram.fill(0);
  packet->pseudogram.fill(0);

  iphdr* iph = packet_ip_header(packet);
  tcphdr* tcph = packet_tcp_header(packet);
  pseudo_header* psh = packet_pseudo_header(packet);

  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = OUTGOING_PACKET_HEADER_LEN;
  iph->id = htonl(std::rand() % 65535);
  iph->frag_off = 0;
  iph->ttl = 64;
  iph->protocol = IPPROTO_TCP;
  iph->check = 0;
  iph->saddr = src->sin_addr.s_addr;
  iph->daddr = dst->sin_addr.s_addr;

  tcph->source = src->sin_port;
  tcph->dest = dst->sin_port;
  tcph->seq = 0;
  tcph->ack_seq = 0;
  tcph->doff = OUTGOING_TCP_HEADER_LEN / 4;
  tcph->fin = 0;
  tcph->syn = 0;
  tcph->rst = 0;
  tcph->psh = push_flag ? 1 : 0;
  tcph->ack = 1;
  tcph->urg = 0;
  tcph->check = 0;
  tcph->window = htons(5840);
  tcph->urg_ptr = 0;

  psh->source_address = src->sin_addr.s_addr;
  psh->dest_address = dst->sin_addr.s_addr;
  psh->placeholder = 0;
  psh->protocol = IPPROTO_TCP;
  psh->tcp_length = htons(OUTGOING_TCP_HEADER_LEN);

  memcpy(packet_pseudo_tcp_segment(packet), tcph, OUTGOING_TCP_HEADER_LEN);

  // iph->check = checksum(packet->datagram.data(), iph->tot_len);
  iph->check = checksum(packet->datagram.data(), IPV4_HEADER_LEN);
}

void prepare_ack_packet(prepared_packet* packet, int32_t seq, int32_t ack_seq,
                        int* out_packet_len) {
  iphdr* iph = packet_ip_header(packet);
  tcphdr* tcph = packet_tcp_header(packet);
  tcphdr* pseudo_tcph = packet_pseudo_tcp_header(packet);

  tcph->seq = htonl(static_cast<uint32_t>(seq));
  tcph->ack_seq = htonl(static_cast<uint32_t>(ack_seq));
  tcph->check = 0;

  // memcpy(packet_pseudo_tcp_segment(packet), tcph, OUTGOING_TCP_HEADER_LEN);
  pseudo_tcph->seq = tcph->seq;
  pseudo_tcph->ack_seq = tcph->ack_seq;
  pseudo_tcph->check = 0;
  tcph->check = checksum(packet->pseudogram.data(),
                         sizeof(pseudo_header) + OUTGOING_TCP_HEADER_LEN);
  pseudo_tcph->check = tcph->check;

  // iph->check = 0;
  // iph->check = checksum(packet->datagram.data(), iph->tot_len);

  *out_packet_len = iph->tot_len;
}

bool prepare_data_packet(prepared_packet* packet, int32_t seq, int32_t ack_seq,
                         const char* data, int data_len, int* out_packet_len) {
  if (data_len < 0 || data_len > MAX_OUTGOING_PAYLOAD_LEN) {
    return false;
  }

  iphdr* iph = packet_ip_header(packet);
  tcphdr* tcph = packet_tcp_header(packet);
  pseudo_header* psh = packet_pseudo_header(packet);

  memcpy(packet_payload(packet), data, static_cast<size_t>(data_len));

  iph->tot_len = OUTGOING_PACKET_HEADER_LEN + data_len;
  iph->check = 0;

  tcph->seq = htonl(static_cast<uint32_t>(seq));
  tcph->ack_seq = htonl(static_cast<uint32_t>(ack_seq));
  tcph->check = 0;

  psh->tcp_length = htons(OUTGOING_TCP_HEADER_LEN + data_len);

  memcpy(packet_pseudo_tcp_segment(packet), tcph, OUTGOING_TCP_HEADER_LEN);
  memcpy(packet_pseudo_tcp_segment(packet) + OUTGOING_TCP_HEADER_LEN, data,
         static_cast<size_t>(data_len));

  tcph->check =
      checksum(packet->pseudogram.data(),
               sizeof(pseudo_header) + OUTGOING_TCP_HEADER_LEN + data_len);
  // iph->check = checksum(packet->datagram.data(), iph->tot_len);
  iph->check = checksum(packet->datagram.data(), IPV4_HEADER_LEN);

  *out_packet_len = iph->tot_len;
  return true;
}

bool has_expected_incoming_layout(const char* packet, int received) {
  if (received < INCOMING_PACKET_HEADER_LEN) {
    return false;
  }

  const int ip_header_length = (packet[0] & 0x0F) * 4;
  if (ip_header_length != IPV4_HEADER_LEN) {
    return false;
  }

  const int tcp_header_length =
      ((packet[IPV4_HEADER_LEN + 12] >> 4) & 0x0F) * 4;
  return tcp_header_length == TCP_BASE_HEADER_LEN;
}

int read_incoming_payload_length(const char* packet, int received) {
  uint16_t total_length = 0;
  memcpy(&total_length, packet + 2, sizeof(total_length));

  const int payload_length = ntohs(total_length) - INCOMING_PACKET_HEADER_LEN;
  // if (payload_length < 0) {
  //   return -1;
  // }
  // if (payload_length > received - INCOMING_PACKET_HEADER_LEN) {
  //   return -1;
  // }
  return payload_length;
}

void create_syn_packet(sockaddr_in* src, sockaddr_in* dst, char** out_packet,
                       int* out_packet_len) {
  char* datagram = static_cast<char*>(calloc(DATAGRAM_LEN, sizeof(char)));

  iphdr* iph = reinterpret_cast<iphdr*>(datagram);
  tcphdr* tcph = reinterpret_cast<tcphdr*>(datagram + sizeof(iphdr));
  pseudo_header psh{};

  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = OUTGOING_PACKET_HEADER_LEN;
  iph->id = htonl(std::rand() % 65535);
  iph->frag_off = 0;
  iph->ttl = 64;
  iph->protocol = IPPROTO_TCP;
  iph->check = 0;
  iph->saddr = src->sin_addr.s_addr;
  iph->daddr = dst->sin_addr.s_addr;

  tcph->source = src->sin_port;
  tcph->dest = dst->sin_port;
  tcph->seq = htonl(static_cast<uint32_t>(std::rand()));
  tcph->ack_seq = htonl(0);
  tcph->doff = OUTGOING_TCP_HEADER_LEN / 4;
  tcph->fin = 0;
  tcph->syn = 1;
  tcph->rst = 0;
  tcph->psh = 0;
  tcph->ack = 0;
  tcph->urg = 0;
  tcph->check = 0;
  tcph->window = htons(5840);
  tcph->urg_ptr = 0;

  psh.source_address = src->sin_addr.s_addr;
  psh.dest_address = dst->sin_addr.s_addr;
  psh.placeholder = 0;
  psh.protocol = IPPROTO_TCP;
  psh.tcp_length = htons(OUTGOING_TCP_HEADER_LEN);

  const int psize = sizeof(pseudo_header) + OUTGOING_TCP_HEADER_LEN;
  char* pseudogram = static_cast<char*>(malloc(psize));
  memcpy(pseudogram, reinterpret_cast<char*>(&psh), sizeof(pseudo_header));
  memcpy(pseudogram + sizeof(pseudo_header), tcph, OUTGOING_TCP_HEADER_LEN);

  datagram[40] = 0x02;
  datagram[41] = 0x04;
  int16_t mss = htons(1440);
  memcpy(datagram + 42, &mss, sizeof(int16_t));
  datagram[44] = 0x04;
  datagram[45] = 0x02;

  pseudogram[32] = 0x02;
  pseudogram[33] = 0x04;
  memcpy(pseudogram + 34, &mss, sizeof(int16_t));
  pseudogram[36] = 0x04;
  pseudogram[37] = 0x02;

  tcph->check = checksum(pseudogram, psize);
  iph->check = checksum(datagram, iph->tot_len);

  *out_packet = datagram;
  *out_packet_len = iph->tot_len;
  free(pseudogram);
}

void read_seq_and_ack(const char* packet, uint32_t* seq, uint32_t* ack) {
  uint32_t seq_num = 0;
  memcpy(&seq_num, packet + 24, 4);
  uint32_t ack_num = 0;
  memcpy(&ack_num, packet + 28, 4);
  *seq = ntohl(seq_num);
  *ack = ntohl(ack_num);
}


// проверить, что не приходит ничего кроме нашего
int receive_from(int sock, char* buffer, size_t buffer_length,
                 const sockaddr_in* dst) {
  (void)dst;

  // Старый вариант: получали все TCP-пакеты в userspace и вручную
  // крутили цикл, пока не найдём пакет на наш локальный порт.
  // unsigned short dst_port = 0;
  // int received = 0;
  // do {
  //   received = recvfrom(sock, buffer, buffer_length, 0, nullptr, nullptr);
  //   if (received < 0) {
  //     break;
  //   }
  //   memcpy(&dst_port, buffer + 22, sizeof(dst_port));
  //   if (dst_port != dst->sin_port) {
  //     std::cout << "received packet from another port (receive_from)" << std::endl;
  //   }
  // } while (dst_port != dst->sin_port);
  // return received;

  // Новый вариант: лишние пакеты уже отфильтрованы BPF в ядре.
  return recvfrom(sock, buffer, buffer_length, 0, nullptr, nullptr);
}

auto get_current_time_mcs() {
  auto now = std::chrono::high_resolution_clock::now();
  auto mcs = std::chrono::duration_cast<std::chrono::microseconds>(
                 now.time_since_epoch())
                 .count();
  return mcs;
}

// обязательно после снятия временной метки
uint64_t read_message_id(const char* message, int payload_length) {
  uint64_t res_id = 0;
  for (int i = 0; i < payload_length; ++i) {
    if (message[i] - '0' < 0 || message[i] - '0' > 9) {
      break;
    }
    res_id = res_id * 10 + (message[i] - '0');
  }
  return res_id;
}

// Навешиваем classic BPF на raw socket, чтобы ядро пропускало только нужный
// входящий TCP-трафик для нашей пары адресов и портов.
bool attach_incoming_socket_filter(int sock, const sockaddr_in* local_addr,
                                   const sockaddr_in* remote_addr) {
  const uint32_t remote_ip = ntohl(remote_addr->sin_addr.s_addr);
  const uint32_t local_ip = ntohl(local_addr->sin_addr.s_addr);
  const uint16_t remote_port = ntohs(remote_addr->sin_port);
  const uint16_t local_port = ntohs(local_addr->sin_port);

  // Смещения внутри IPv4-пакета без опций:
  // 9  -> protocol
  // 12 -> source IPv4
  // 16 -> destination IPv4
  // 20 -> TCP source port
  // 22 -> TCP destination port
  sock_filter filter_code[] = {
      // Оставляем только TCP.
      BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 9),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_TCP, 0, 9),

      // Проверяем, что пакет пришёл от нужного удалённого IPv4.
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 12),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, remote_ip, 0, 7),

      // Проверяем, что пакет адресован нашему локальному IPv4.
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 16),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, local_ip, 0, 5),

      // Проверяем удалённый TCP source port.
      BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 20),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, remote_port, 0, 3),

      // Проверяем наш локальный TCP destination port.
      BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 22),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, local_port, 0, 1),

      // Пакет подходит под фильтр: отдаём его сокету.
      BPF_STMT(BPF_RET | BPF_K, 0xFFFFFFFFu),

      // Пакет не подходит: отбрасываем в ядре.
      BPF_STMT(BPF_RET | BPF_K, 0),
  };

  sock_fprog filter_program{};
  filter_program.len =
      static_cast<unsigned short>(sizeof(filter_code) / sizeof(filter_code[0]));
  filter_program.filter = filter_code;

  return setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter_program,
                    sizeof(filter_program)) == 0;
}


// inline функции, лучше оформить код без вызовов функций
// можно попробовать [[always_inline]] прописать все функции в main
// то есть вообще без вызовов функций
int main(int argc, char** argv) {
  // Старый вариант: сохраняли пару {echoed timestamp, время получения}.
  // std::vector<std::pair<uint64_t, uint64_t>> timestamps;
  // timestamps.reserve(5000);
  std::vector<uint64_t> deltas;
  deltas.reserve(MESSAGE_COUNT);


  if (geteuid() != 0) {
    std::cout << "TCP client must be run as sudo!" << std::endl;
    return 1;
  }

  std::srand(static_cast<unsigned>(std::time(nullptr)));

  std::string source_ip = "192.168.3.13";
  std::string target_ip = "192.168.3.2";
  int target_port = 3333;
  if (argc >= 2) {
    source_ip = argv[1];
  }
  if (argc >= 3) {
    target_ip = argv[2];
  }
  if (argc >= 4) {
    target_port = std::atoi(argv[3]);
  }

  std::cout << "source_ip = " << source_ip << std::endl;
  std::cout << "target_ip = " << target_ip << std::endl;
  std::cout << "target_port = " << target_port << std::endl;

  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
  if (sock == -1) {
    std::cerr << "socket creation failed\n";
    return 1;
  }

  timeval tv{};
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  sockaddr_in daddr{};
  daddr.sin_family = AF_INET;
  daddr.sin_port = htons(static_cast<uint16_t>(target_port));
  if (inet_pton(AF_INET, target_ip.c_str(), &daddr.sin_addr) != 1) {
    std::cerr << "destination IP configuration failed\n";
    close(sock);
    return 1;
  }

  sockaddr_in saddr{};
  saddr.sin_family = AF_INET;
  saddr.sin_port =
      htons(static_cast<uint16_t>(1024 + (std::rand() % (65535 - 1024))));
  if (inet_pton(AF_INET, source_ip.c_str(), &saddr.sin_addr) != 1) {
    std::cerr << "source IP configuration failed\n";
    close(sock);
    return 1;
  }

  int one = 1;
  if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1) {
    std::cerr << "setsockopt(IP_HDRINCL, 1) failed\n";
    close(sock);
    return 1;
  }

  // Новый вариант: ядро само отбросит весь TCP-трафик, который не относится
  // к нашей паре {remote ip:port -> local ip:port}.
  if (!attach_incoming_socket_filter(sock, &saddr, &daddr)) {
    std::cerr << "setsockopt(SO_ATTACH_FILTER) failed\n";
    close(sock);
    return 1;
  }

  char* packet = nullptr;
  int packet_len = 0;
  prepared_packet ack_packet;
  prepared_packet data_packet;

  std::cout << "Client source port: " << ntohs(saddr.sin_port) << '\n';

  // ---------- SYN ----------
  create_syn_packet(&saddr, &daddr, &packet, &packet_len);
  int sent = sendto(sock, packet, packet_len, 0,
                    reinterpret_cast<sockaddr*>(&daddr), sizeof(sockaddr));
  free(packet);
  if (sent == -1) {
    std::cerr << "send SYN failed\n";
    close(sock);
    return 1;
  }
  std::cout << "SYN sent\n";

  // -------- SYN-ACK --------
  char recvbuf[DATAGRAM_LEN];
  int received = receive_from(sock, recvbuf, sizeof(recvbuf), &saddr);
  std::cout << "SYN-ACK recieved from " << sock << std::endl;
  if (received <= 0) {
    std::cerr << "failed to receive SYN-ACK\n";
    close(sock);
    return 1;
  }

  uint32_t server_seq = 0;
  uint32_t server_ack = 0;
  read_seq_and_ack(recvbuf, &server_seq, &server_ack);
  int32_t client_seq = static_cast<int32_t>(server_ack);
  int32_t ack_to_server = static_cast<int32_t>(server_seq + 1);

  std::cout << "client_seq = " << client_seq << std::endl;

  // ---------- ACK ----------
  initialize_base_packet(&ack_packet, &saddr, &daddr, false);
  initialize_base_packet(&data_packet, &saddr, &daddr, true);

  prepare_ack_packet(&ack_packet, client_seq, ack_to_server, &packet_len);
  sent = sendto(sock, ack_packet.datagram.data(), packet_len, 0,
                reinterpret_cast<sockaddr*>(&daddr), sizeof(sockaddr));
  if (sent == -1) {
    std::cerr << "send ACK failed\n";
    close(sock);
    return 1;
  }
  std::cout << "ACK sent\n";
  std::cout << "Handshake complete\n";

  // ------ DUPLEX LOOP ------

  // Старый вариант: клиент только принимал поток сообщений от сервера.
  // std::cout << "start_receiving" << std::endl;
  // uint32_t seq_number = 0;
  // uint32_t ack_number = 0;
  // int payload_length;
  // uint64_t cur_time;
  // const char* payload;
  // while (true) {
  //   received = receive_from(sock, recvbuf, sizeof(recvbuf), &saddr);
  //   if (received <= 0) [[unlikely]] {
  //     break;
  //   }
  //   cur_time = std::chrono::duration_cast<std::chrono::microseconds>(
  //       std::chrono::steady_clock::now().time_since_epoch()
  //   ).count();
  //   payload = recvbuf + INCOMING_PAYLOAD_OFFSET;
  //   payload_length = read_incoming_payload_length(recvbuf, received);
  //   timestamps.emplace_back(read_message_id(payload, payload_length), cur_time);
  //   read_seq_and_ack(recvbuf, &seq_number, &ack_number);
  //   ack_to_server = seq_number + payload_length;
  //   prepare_ack_packet(&ack_packet, client_seq, ack_to_server, &packet_len);
  //   sent = sendto(sock, ack_packet.datagram.data(), packet_len, 0,
  //                 reinterpret_cast<sockaddr*>(&daddr), sizeof(sockaddr));
  //   if (static_cast<unsigned char>(recvbuf[INCOMING_FLAGS_OFFSET]) & 0x01) [[unlikely]] {
  //     std::cout << "Received FIN, closing connection" << std::endl;
  //     break;
  //   }
  //   if (sent <= 0) [[unlikely]] {
  //     break;
  //   }
  // }

  std::cout << "start_duplex" << std::endl;

  uint32_t seq_number = 0;
  uint32_t ack_number = 0;
  int payload_length = 0;
  const char* payload = nullptr;
  bool failed = false;

  for (int i = 0; i < MESSAGE_COUNT; ++i) {
    const uint64_t send_time = get_current_time_mcs();
    const std::string message = std::to_string(send_time) + '!';
    const int message_len = static_cast<int>(message.size());

    if (!prepare_data_packet(&data_packet, client_seq, ack_to_server,
                             message.data(), message_len, &packet_len)) {
      std::cerr << "Failed to prepare duplex payload packet\n";
      failed = true;
      break;
    }

    sent = sendto(sock, data_packet.datagram.data(), packet_len, 0,
                  reinterpret_cast<sockaddr*>(&daddr), sizeof(sockaddr));
    if (sent <= 0) [[unlikely]] {
      failed = true;
      break;
    }

    client_seq += message_len;

    while (true) {
      received = receive_from(sock, recvbuf, sizeof(recvbuf), &saddr);
      if (received <= 0) [[unlikely]] {
        failed = true;
        break;
      }

      read_seq_and_ack(recvbuf, &seq_number, &ack_number);
      payload_length = read_incoming_payload_length(recvbuf, received);
      const bool fin_received =
          (static_cast<unsigned char>(recvbuf[INCOMING_FLAGS_OFFSET]) & 0x01) != 0;

      if (payload_length <= 0) {
        if (fin_received) {
          ack_to_server = static_cast<int32_t>(seq_number + 1);
          prepare_ack_packet(&ack_packet, client_seq, ack_to_server, &packet_len);
          sent = sendto(sock, ack_packet.datagram.data(), packet_len, 0,
                        reinterpret_cast<sockaddr*>(&daddr), sizeof(sockaddr));
          failed = true;
        }
        if (sent <= 0) {
          failed = true;
        }
        if (failed) {
          break;
        }
        continue;
      }

      payload = recvbuf + INCOMING_PAYLOAD_OFFSET;
      const uint64_t receive_time = get_current_time_mcs();
      const uint64_t echoed_send_time = read_message_id(payload, payload_length);
      deltas.push_back(receive_time - echoed_send_time);

      ack_to_server =
          static_cast<int32_t>(seq_number + payload_length + (fin_received ? 1 : 0));
      prepare_ack_packet(&ack_packet, client_seq, ack_to_server, &packet_len);
      sent = sendto(sock, ack_packet.datagram.data(), packet_len, 0,
                    reinterpret_cast<sockaddr*>(&daddr), sizeof(sockaddr));
      if (sent <= 0) [[unlikely]] {
        failed = true;
      }
      if (fin_received) {
        failed = true;
      }
      break;
    }

    if (failed) {
      break;
    }
  }

  std::ofstream fout("recv_duo_raw.txt");

  // Старый вариант: писали пару {echoed timestamp, время получения}.
  // for (const auto [first_time, second_time] : timestamps) {
  //   fout << first_time << ' ' << second_time << '\n';
  // }
  for (const uint64_t delta : deltas) {
    fout << delta << '\n';
  }

  fout.close();

  close(sock);

  std::cout << "Socket closed\n";

  return 0;
}

/*
USAGE:
g++ server.cpp -o server.exe -std=c++23 -O3 -pthread -lboost_system -Wall -Wextra -pedantic
g++ client.cpp -o client.exe -std=c++23 -O3 -Wall -Wextra -pedantic

sudo setcap cap_net_raw+ep ./client.exe

sudo ip netns add ns_c
sudo ip netns add ns_s
sudo ip link add veth_c type veth peer name veth_s
sudo ip link set veth_c netns ns_c
sudo ip link set veth_s netns ns_s
sudo ip netns exec ns_c ip addr add 10.200.1.1/24 dev veth_c
sudo ip netns exec ns_s ip addr add 10.200.1.2/24 dev veth_s
sudo ip netns exec ns_c ip link set lo up
sudo ip netns exec ns_s ip link set lo up
sudo ip netns exec ns_c ip link set veth_c up
sudo ip netns exec ns_s ip link set veth_s up

sudo ip netns exec ns_c iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP
IMPORTANT!!!

FIRST TERMINAL:
sudo ip netns exec ns_s ./server.exe
SECOND TERMINAL:
sudo ip netns exec ns_c ./client.exe 10.200.1.1 10.200.1.2 3333
CHECK:
sudo ip netns exec ns_c tcpdump -n -i veth_c tcp port 3333

Kill namespaces:
sudo ip netns delete ns_c
sudo ip netns delete ns_s

addition

sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -d 172.20.10.3 -j DROP
*/
