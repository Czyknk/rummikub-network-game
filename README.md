# Networked Rummikub

A multiplayer, turn-based strategy game implementation based on the Rummikub ruleset. Developed as a final project for the **Computer Networks** course. The project demonstrates a socket implementation in C++ (Server) and a graphical client in Python (Pygame).

## 🚀 Project Overview

The system operates on a Client-Server architecture using TCP/IP sockets. It supports concurrent game rooms (2-4 players) and enforces strict server-side validation of game logic.

### Key Features
* **Multiplayer Support:** Dynamic lobby system handling queues for 2, 3, or 4 players.
* **Concurrency:** Event-driven server design using Linux `poll()` API (I/O Multiplexing) to handle multiple clients on a single thread.
* **Game Logic Validation:** Full server-side verification of Rummikub rules (runs, groups, jokers, and initial meld points).
* **Transactional State Management:** Implements a "commit/rollback" mechanism. Invalid moves trigger a state rollback to ensure game integrity.
* **Resilience:** Handling of TCP stream fragmentation (client-side) and graceful handling of unexpected client disconnections.

## 🛠️ Technology Stack

### Server
* **Language:** C++14
* **OS Target:** GNU/Linux
* **Networking:** BSD Sockets, Non-blocking I/O (`O_NONBLOCK`), `sys/poll.h`.
* **Architecture:** Single-threaded, Event-Driven.

### Client
* **Language:** Python 3.x
* **GUI Library:** Pygame
* **Networking:** `socket` and `threading` modules (asynchronous network listener).

## ⚙️ Installation & Build

### Prerequisites
* **Linux Environment** (for Server compilation) with `g++` and `make`.
* **Python 3.6+** installed.
* **Pygame** library.

### 1. Building the Server
The project includes a `Makefile` for automated compilation.

To build the server, simply run:

```bash
make
