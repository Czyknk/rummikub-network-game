#ifndef SHARED_HPP
#define SHARED_HPP

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <random>

#include "../include/Protocol.hpp"

// Stałe
const int DEFAULT_PORT = 1100;
const int BUFFER_SIZE = 4096;
const int POLL_TIMEOUT = 1000; // ms 
const int INITIAL_HAND_SIZE = 14;


enum TileColor { RED = 0, BLUE = 1, BLACK = 2, YELLOW = 3, JOKER = 4 };

struct Tile {
    int id;         
    int value;      // 1-13, 0 dla Jokera
    int color;      // 0-3, 4 dla Jokera
    int x = -1;     // Pozycja X na stole
    int y = -1;     // Pozycja Y na stole

    // do debugowania i wysyłania
    std::string toString() const {
        // Format: ID:COLOR:VALUE (np. "105:0:13")
        return std::to_string(id) + ":" + std::to_string(color) + ":" + std::to_string(value);
    }

    // Pełna reprezentacja (z pozycją) do BOARD_UPDATE
    std::string toFullString() const {
        return std::to_string(id) + ":" + std::to_string(x) + ":" + std::to_string(y) + ":" + 
               std::to_string(color) + ":" + std::to_string(value);
    }
};

inline int calculateHandPoints(const std::vector<Tile>& hand) {
    int points = 0;
    for (const auto& t : hand) {
        if (t.value == 0) points += 30; // Joker
        else points += t.value;
    }
    return points;
}

#endif