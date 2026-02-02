#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>


#include "Client.hpp"
#include "Shared.hpp"
#include "Room.hpp"

class GameServer {
private:
    int SocketFD;
    int port;
    int next_room_id; // Licznik ID pokoi

    std::vector<struct pollfd> poll_set;
    std::map<int, Client*> clients;
    
    // Zarządzanie pokojami
    std::map<int, Room*> rooms;
    
    // Kolejki Lobby (gracze czekający na 2, 3 lub 4 osoby)
    std::vector<Client*> queue_2;
    std::vector<Client*> queue_3;
    std::vector<Client*> queue_4;

    void setNonBlocking(int fd);
    void handleNewConnection();
    void handleClientData(int index);
    void removeClient(int index);
    void removeFromVector(std::vector<Client*>& vec, Client* target);

    // Nowe metody
    void processCommand(Client* client, std::string cmd);
    void addToLobby(Client* client, int mode);
    void createRoom(std::vector<Client*>& queue, int mode);

public:
    GameServer(int port_num);
    ~GameServer();
    void run();
};

#endif