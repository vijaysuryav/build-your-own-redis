#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <cassert>
#include <map>
#include "hashtable.h"
#include <unordered_map>
#include <ctime>  // for time()


#include <map>
std::unordered_map<std::string, std::map<std::string, double>> g_zsets;

struct Entry {
    HNode node;
    std::string key;
    std::string val;
};

uint64_t str_hash(const char* data, size_t len) {
    uint64_t h = 5381;
    for (size_t i = 0; i < len; ++i)
        h = ((h << 5) + h) ^ data[i];
    return h;
}

bool entry_eq(HNode* a, HNode* b) {
    Entry* ea = (Entry*)a;
    Entry* eb = (Entry*)b;
    return ea->key == eb->key;
}

HTab g_table;
std::unordered_map<std::string, uint64_t> g_expire;

const size_t k_max_msg = 4096;

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2
};

struct Conn {
    int fd = -1;
    uint32_t state = STATE_REQ;
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};
bool is_expired(const std::string& key) {
    auto it = g_expire.find(key);
    if (it == g_expire.end()) return false;

    uint64_t now = time(nullptr);
    if (now >= it->second) {
        Entry temp;
        temp.key = key;
        temp.node.hcode = str_hash(key.data(), key.size());
        HNode** found = h_lookup(&g_table, &temp.node, entry_eq);
        if (found) {
            delete (Entry*)h_detach(&g_table, found);
        }
        g_expire.erase(it);
        return true;
    }
    return false;
}
void msg(const char* s) { std::cout << s << std::endl; }

void die(const char* s) {
    perror(s);
    exit(1);
}

void fd_set_nb(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) die("fcntl get");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) die("fcntl set");
}

Conn* conn_new(int fd) {
    Conn* conn = new Conn();
    conn->fd = fd;
    return conn;
}

void conn_put(std::vector<Conn*>& fd2conn, Conn* conn) {
    if (fd2conn.size() <= (size_t)conn->fd)
        fd2conn.resize(conn->fd + 1);
    fd2conn[conn->fd] = conn;
}

int32_t accept_new_conn(std::vector<Conn*>& fd2conn, int fd) {
    sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (sockaddr*)&client_addr, &socklen);
    if (connfd < 0) return -1;
    fd_set_nb(connfd);
    Conn* conn = conn_new(connfd);
    conn_put(fd2conn, conn);
    return 0;
}



bool try_one_request(Conn* conn) {
    if (conn->rbuf_size < 4) return false;

    uint32_t len = 0;
    memcpy(&len, conn->rbuf, 4);
    if (len > k_max_msg) {
        conn->state = STATE_END;
        return false;
    }

    if (conn->rbuf_size < 4 + len) return false;

    std::vector<std::string> parts;
    uint32_t nstr = 0;
    memcpy(&nstr, conn->rbuf + 4, 4);
    size_t pos = 8;

    for (uint32_t i = 0; i < nstr; ++i) {
        if (pos + 4 > 4 + len) break;
        uint32_t slen = 0;
        memcpy(&slen, conn->rbuf + pos, 4);
        pos += 4;
        if (pos + slen > 4 + len) break;
        parts.emplace_back((char*)conn->rbuf + pos, slen);
        pos += slen;
    }

    std::string response;
    uint32_t rescode = 0;

    std::cout << "Parsed command: ";
    for (const auto& p : parts) std::cout << "[" << p << "] ";
    std::cout << std::endl;

    if (parts.size() == 3 && parts[0] == "set") {
        Entry key;
        key.key = parts[1];
        key.node.hcode = str_hash(key.key.data(), key.key.size());

        HNode** found = h_lookup(&g_table, &key.node, entry_eq);
        if (found) {
            ((Entry*)(*found))->val = parts[2];
        } else {
            if (g_table.size >= g_table.mask + 1) {
                h_resize(&g_table, (g_table.mask + 1) * 2);
            }
            Entry* ent = new Entry();
            ent->key = parts[1];
            ent->val = parts[2];
            ent->node.hcode = key.node.hcode;
            h_insert(&g_table, &ent->node);
        }
        response = "OK";
        rescode = 0;

    } else if (parts.size() == 2 && parts[0] == "get") {
        if (is_expired(parts[1])) {
            response = "(nil)";
            rescode = 2;
        } else {
            Entry key;
            key.key = parts[1];
            key.node.hcode = str_hash(key.key.data(), key.key.size());
            HNode** found = h_lookup(&g_table, &key.node, entry_eq);
            if (found) {
                Entry* ent = (Entry*)(*found);
                response = ent->val;
                rescode = 0;
            } else {
                response = "(nil)";
                rescode = 2;
            }
        }

    } else if (parts.size() == 2 && parts[0] == "del") {
        if (is_expired(parts[1])) {
            response = "(nil)";
            rescode = 2;
        } else {
            Entry key;
            key.key = parts[1];
            key.node.hcode = str_hash(key.key.data(), key.key.size());
            HNode** found = h_lookup(&g_table, &key.node, entry_eq);
            if (found) {
                delete (Entry*)h_detach(&g_table, found);
                g_expire.erase(key.key);
                response = "OK";
                rescode = 0;
            } else {
                response = "(nil)";
                rescode = 2;
            }
        }

    } else if (parts.size() == 1 && parts[0] == "keys") {
        std::string result;
        for (size_t i = 0; i <= g_table.mask; ++i) {
            HNode* node = g_table.tab[i];
            while (node) {
                Entry* ent = (Entry*)node;
                if (!is_expired(ent->key)) {
                    result += ent->key + "\n";
                }
                node = node->next;
            }
        }
        response = result.empty() ? "(empty)" : result;
        rescode = 0;

    } else if (parts.size() == 3 && parts[0] == "expire") {
        const std::string& key = parts[1];
        int seconds = std::stoi(parts[2]);
        Entry temp;
        temp.key = key;
        temp.node.hcode = str_hash(key.data(), key.size());
        HNode** found = h_lookup(&g_table, &temp.node, entry_eq);
        if (found) {
            g_expire[key] = time(nullptr) + seconds;
            response = "OK";
            rescode = 0;
        } else {
            response = "(nil)";
            rescode = 2;
        }

    } else if (parts.size() == 2 && parts[0] == "ttl") {
        const std::string& key = parts[1];
        if (is_expired(key)) {
            response = "(nil)";
            rescode = 2;
        } else {
            auto it = g_expire.find(key);
            if (it == g_expire.end()) {
                response = "-1";  // No TTL set
            } else {
                int64_t remaining = (int64_t)it->second - (int64_t)time(nullptr);
                response = std::to_string(remaining > 0 ? remaining : 0);
            }
            rescode = 0;
        }

    }else if (parts.size() == 4 && parts[0] == "zadd") {
    const std::string& key = parts[1];
    double score = std::stod(parts[2]);
    const std::string& member = parts[3];

    g_zsets[key][member] = score;
    response = "OK";
    rescode = 0;
    }else if (parts.size() == 3 && parts[0] == "zscore") {
        const std::string& key = parts[1];
        const std::string& member = parts[2];

        auto key_it = g_zsets.find(key);
        if (key_it != g_zsets.end()) {
            auto member_it = key_it->second.find(member);
            if (member_it != key_it->second.end()) {
                response = std::to_string(member_it->second);
                rescode = 0;
            } else {
                response = "(nil)";
                rescode = 2;
            }
        } else {
            response = "(nil)";
            rescode = 2;
        }
    }else {
        std::cerr << "Unknown command: ";
        for (const auto& p : parts) std::cerr << "[" << p << "] ";
        std::cerr << std::endl;

        response = "Unknown command";
        rescode = 1;
    }

    // Serialize response
    uint32_t wlen = (uint32_t)response.size() + 4;
    memcpy(conn->wbuf, &wlen, 4);
    memcpy(conn->wbuf + 4, &rescode, 4);
    memcpy(conn->wbuf + 8, response.data(), response.size());
    conn->wbuf_size = 4 + wlen;

    // Consume request from read buffer
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) memmove(conn->rbuf, conn->rbuf + 4 + len, remain);
    conn->rbuf_size = remain;

    conn->state = STATE_RES;
    return true;
}




bool try_fill_buffer(Conn* conn) {
    ssize_t rv = 0;
    while (true) {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, conn->rbuf + conn->rbuf_size, cap);
        if (rv < 0 && errno == EINTR) continue;
        break;
    }

    if (rv < 0 && errno == EAGAIN) return false;
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) msg("unexpected EOF");
        else msg("EOF");
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    while (try_one_request(conn)) {}
    return (conn->state == STATE_REQ);
}

bool try_flush_buffer(Conn* conn) {
    ssize_t rv = 0;
    while (true) {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, conn->wbuf + conn->wbuf_sent, remain);
        if (rv < 0 && errno == EINTR) continue;
        break;
    }

    if (rv < 0 && errno == EAGAIN) return false;
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }

    conn->wbuf_sent += (size_t)rv;
    if (conn->wbuf_sent == conn->wbuf_size) {
        conn->wbuf_sent = conn->wbuf_size = 0;
        conn->state = STATE_REQ;
        return false;
    }
    return true;
}

void connection_io(Conn* conn) {
    if (conn->state == STATE_REQ) {
        try_fill_buffer(conn);
    } else if (conn->state == STATE_RES) {
        try_flush_buffer(conn);
    } else {
        assert(0);
    }
}




int main() {
    h_init(&g_table, 1024);  // power of 2
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) die("socket()");
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (sockaddr*)&addr, sizeof(addr))) die("bind()");
    if (listen(fd, SOMAXCONN)) die("listen()");

    fd_set_nb(fd);
    std::vector<Conn*> fd2conn;

    std::cout << "Server (poll) listening on port 1234..." << std::endl;

while (true) {
    std::vector<pollfd> poll_args;

    // Listening socket with revents explicitly set
    pollfd listen_pfd = {};
    listen_pfd.fd = fd;
    listen_pfd.events = POLLIN;
    listen_pfd.revents = 0;
    poll_args.push_back(listen_pfd);

    for (Conn* conn : fd2conn) {
        if (!conn) continue;

        pollfd client_pfd = {};
        client_pfd.fd = conn->fd;
        client_pfd.events = static_cast<short>(conn->state == STATE_REQ ? POLLIN : POLLOUT);
        client_pfd.revents = 0;
        poll_args.push_back(client_pfd);
    }

    int rv = poll(poll_args.data(), poll_args.size(), 1000);
    if (rv < 0) die("poll()");

        if (poll_args[0].revents) {
            (void)accept_new_conn(fd2conn, fd);
        }

        for (size_t i = 1; i < poll_args.size(); ++i) {
            if (!poll_args[i].revents) continue;
            Conn* conn = fd2conn[poll_args[i].fd];
            connection_io(conn);
            if (conn->state == STATE_END) {
                fd2conn[conn->fd] = nullptr;
                close(conn->fd);
                delete conn;
            }
        }
    }
    return 0;
}
