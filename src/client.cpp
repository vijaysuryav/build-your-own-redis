// src/client.cpp
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cerrno>


const size_t k_max_msg = 4096;

void die(const char* msg) {
    perror(msg);
    exit(1);
}
int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) return -1;
        buf += rv;
        n -= rv;
    }
    return 0;
}

int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) return -1;
        buf += rv;
        n -= rv;
    }
    return 0;
}

int32_t query(int fd, const char *text) {
    uint32_t len = strlen(text);
    if (len > k_max_msg) return -1;

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(wbuf + 4, text, len);

    if (write_all(fd, wbuf, 4 + len)) return -1;

    char rbuf[4 + k_max_msg + 1];
    int32_t err = read_full(fd, rbuf, 4);
    if (err) return err;

    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) return -1;

    err = read_full(fd, rbuf + 4, len);
    if (err) return err;

    rbuf[4 + len] = '\0';
    std::cout << "server says: " << (rbuf + 4) << std::endl;
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./client <cmd> [args...]\n";
        return 1;
    }

    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i)
        cmd.push_back(argv[i]);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) die("socket()");

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) die("connect()");

    // Serialize command
    uint32_t total_len = 4;
    for (const auto& s : cmd) total_len += 4 + s.size();

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &total_len, 4);
    uint32_t n = cmd.size();
    memcpy(wbuf + 4, &n, 4);
    size_t cur = 8;
    for (const auto& s : cmd) {
        uint32_t len = s.size();
        memcpy(wbuf + cur, &len, 4);
        memcpy(wbuf + cur + 4, s.data(), len);
        cur += 4 + len;
    }

    write_all(fd, wbuf, 4 + total_len);

    // Receive response
    char rbuf[4 + k_max_msg + 1];
    read_full(fd, rbuf, 4);
    uint32_t rlen = 0;
    memcpy(&rlen, rbuf, 4);
    read_full(fd, rbuf + 4, rlen);
    uint32_t code = 0;
    memcpy(&code, rbuf + 4, 4);
    rbuf[4 + rlen] = '\0';

    std::cout << "server says [" << code << "]: " << (rbuf + 8) << "\n";

    close(fd);
    return 0;
}









// // src/client.cpp
// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <cstring>
// #include <iostream>
// #include <cassert>

// const size_t k_max_msg = 4096;

// int32_t read_full(int fd, char *buf, size_t n) {
//     while (n > 0) {
//         ssize_t rv = read(fd, buf, n);
//         if (rv <= 0) return -1;
//         buf += rv;
//         n -= rv;
//     }
//     return 0;
// }

// int32_t write_all(int fd, const char *buf, size_t n) {
//     while (n > 0) {
//         ssize_t rv = write(fd, buf, n);
//         if (rv <= 0) return -1;
//         buf += rv;
//         n -= rv;
//     }
//     return 0;
// }

// int32_t query(int fd, const char *text) {
//     uint32_t len = strlen(text);
//     if (len > k_max_msg) return -1;

//     char wbuf[4 + k_max_msg];
//     memcpy(wbuf, &len, 4);
//     memcpy(wbuf + 4, text, len);

//     if (write_all(fd, wbuf, 4 + len)) return -1;

//     char rbuf[4 + k_max_msg + 1];
//     int32_t err = read_full(fd, rbuf, 4);
//     if (err) return err;

//     memcpy(&len, rbuf, 4);
//     if (len > k_max_msg) return -1;

//     err = read_full(fd, rbuf + 4, len);
//     if (err) return err;

//     rbuf[4 + len] = '\0';
//     std::cout << "server says: " << (rbuf + 4) << std::endl;
//     return 0;
// }

// int main() {
//     int fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (fd < 0) { perror("socket()"); return 1; }

//     sockaddr_in addr = {};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(1234);
//     addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

//     if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
//         perror("connect()"); return 1;
//     }

//     query(fd, "hello1");
//     query(fd, "hello2");
//     query(fd, "hello3");

//     close(fd);
//     return 0;
// }
