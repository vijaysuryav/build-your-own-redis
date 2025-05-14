# 🧠 Build Your Own Redis (C++) — Sorted Sets Edition

This is a Redis clone built entirely in C++ from scratch, extended with support for **sorted sets** (`zadd`, `zscore`, `zrange`, `zincrby`) — essential for leaderboards, rankings, and financial analytics.

> 🔧 Branch: `sorted-sets`  
> 📘 Inspired by [build-your-own.org/redis](https://build-your-own.org/redis/)

---

## ✨ Features Implemented

- 🧵 TCP server-client communication via raw sockets
- 🔁 Event-driven I/O loop using `poll()`
- 🧠 Custom binary protocol (length-prefixed)
- 🗃️ In-memory hash table with chaining + dynamic resizing
- 🧹 TTL-based expiration (`expire`, `ttl`) with lazy eviction
- 🔢 Sorted Set support with:
  - `zadd`, `zscore`, `zrange`, `zincrby`
- 📦 Redis-style CLI command interface

---

## 🧪 Supported Commands

| Command                           | Description                                         |
|----------------------------------|-----------------------------------------------------|
| `set <key> <value>`              | Store a key-value pair                              |
| `get <key>`                      | Retrieve a value                                    |
| `del <key>`                      | Delete a key                                        |
| `keys`                           | List all stored keys                                |
| `expire <key> <seconds>`         | Set a TTL on a key                                  |
| `ttl <key>`                      | Get remaining time-to-live for a key                |
| `zadd <key> <score> <member>`    | Add or update a member in a sorted set              |
| `zscore <key> <member>`          | Get the score of a member in a sorted set           |
| `zrange <key> <start> <end>`     | Get members in score-sorted order by index range    |
| `zincrby <key> <delta> <member>` | Increment a member's score by a given delta         |

---

## 🛠️ How to Build

```bash
make
```

## 🚀 How to Run
Start the server in one terminal:
```bash
./server
```
Then in a separate terminal, run client commands:
```bash
./client set foo bar
./client get foo
./client expire foo 5
./client ttl foo
sleep 5
./client get foo     # (nil) — expired!
```
## 🔢 Sorted Set Usage Examples
```bash
./client zadd leaderboard 1000 alice
./client zadd leaderboard 1500 bob
./client zadd leaderboard 1200 charlie

./client zscore leaderboard alice
# -> 1000.000000

./client zrange leaderboard 0 -1
# -> alice
#    charlie
#    bob

./client zincrby leaderboard 250 alice
# -> 1250.000000

./client zrange leaderboard 0 -1
# -> charlie
#    bob
#    alice

```
## 📚 Learning Outcomes
Designed a binary protocol over TCP from scratch

Implemented ordered maps and TTL expiration

Built a non-blocking Redis-style event loop using poll()

Simulated Redis sorted set logic for ranking systems

## 🔭 Optional Future Additions
zrank / zrevrank — Get member’s rank by score

zrangebyscore — Return members between score bounds

flushall — Clear entire keyspace

Snapshot or AOF-based persistence

Background TTL eviction (see background-ttl branch)


