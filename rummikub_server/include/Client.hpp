#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Shared.hpp"

struct Client {
    int fd;
    std::string ip_address;
    // std::string buffer;
    
    int room_id;        // -1 Lobby, >= 0 to ID pokoju
    std::string nick;
    std::vector<Tile> hand;

    bool has_initial_meld = false; // Czy gracz wyłożył 30 pkt

    Client(int socket_fd, std::string ip) 
        : fd(socket_fd), ip_address(ip), room_id(-1), nick("Player"), has_initial_meld(false) {}

    void clearHand() {
        hand.clear();
        has_initial_meld = false; // Reset przy nowej grz
    }
};
#endif