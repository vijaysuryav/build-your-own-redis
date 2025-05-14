// src/server.cpp
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cassert>

const size_t k_max_msg = 4096;

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

int32_t one_request(int connfd) {
    char rbuf[4 + k_max_msg + 1];
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) return err;

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) return -1;

    err = read_full(connfd, rbuf + 4, len);
    if (err) return err;

    rbuf[4 + len] = '\0';
    std::cout << "client says: " << (rbuf + 4) << std::endl;

    const char *reply = "world";
    len = strlen(reply);
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(wbuf + 4, reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

void handle_client(int connfd) {
    while (true) {
        if (one_request(connfd)) break;
    }
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket()"); return 1; }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind()"); return 1;
    }

    if (listen(fd, SOMAXCONN) < 0) {
        perror("listen()"); return 1;
    }

    std::cout << "Server listening on port 1234..." << std::endl;

    while (true) {
        sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (sockaddr *)&client_addr, &socklen);
        if (connfd < 0) continue;

        handle_client(connfd);
        close(connfd);
    }

    return 0;
}




// // src/server.cpp
// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <cstring>
// #include <iostream>

// void die(const char *msg) {
//     perror(msg);
//     exit(EXIT_FAILURE);
// }

// void handle_client(int connfd) {
//     char buf[64] = {};
//     ssize_t n = read(connfd, buf, sizeof(buf) - 1);
//     if (n < 0) {
//         perror("read()");
//         return;
//     }

//     std::cout << "client says: " << buf << std::endl;
//     const char *reply = "world";
//     write(connfd, reply, strlen(reply));
// }

// int main() {
//     int fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (fd < 0) die("socket()");

//     int opt = 1;
//     setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//     sockaddr_in addr = {};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(1234);
//     addr.sin_addr.s_addr = htonl(INADDR_ANY);

//     if (bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0) die("bind()");
//     if (listen(fd, SOMAXCONN) < 0) die("listen()");

//     std::cout << "Server listening on port 1234...\n";
//     while (true) {
//         sockaddr_in client_addr = {};
//         socklen_t socklen = sizeof(client_addr);
//         int connfd = accept(fd, (sockaddr *)&client_addr, &socklen);
//         if (connfd < 0) continue;
//         handle_client(connfd);
//         close(connfd);
//     }

//     close(fd);
//     return 0;
// }
