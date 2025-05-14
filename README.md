# 🧠 Build Your Own Redis (C++)

This is a minimal Redis clone built entirely in C++ from scratch.  
It closely follows the internals of Redis to demonstrate how a real-time key-value store works under the hood.

> 📘 Inspired by [build-your-own.org/redis](https://build-your-own.org/redis/)

---

## ✨ Features Implemented

- 🧵 **TCP server-client communication** using sockets
- 🧠 **Custom binary protocol** (length-prefixed)
- 🔁 **Event-driven server** using `poll()` for concurrent clients
- 🗃️ **In-memory hash table** with chaining and dynamic resizing
- 🧹 **Lazy TTL-based expiration**
- 📦 Redis-style commands:

| Command                     | Description                                 |
|-----------------------------|---------------------------------------------|
| `set <key> <value>`         | Store a key-value pair                      |
| `get <key>`                 | Retrieve a value                           |
| `del <key>`                 | Delete a key                               |
| `keys`                      | List all active keys                       |
| `expire <key> <seconds>`    | Set a TTL on a key                         |
| `ttl <key>`                 | Get remaining time before expiration       |

---

## 🛠️ How to Build

```bash
make

🚀 How to Run
Start the server in one terminal: ./server

Then, in a separate terminal, run client commands:
./client set foo bar
./client get foo
./client expire foo 5
./client ttl foo
sleep 5
./client get foo     # (nil) — expired!
