#ifndef ROOM_HPP
#define ROOM_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include "Client.hpp"
#include "Shared.hpp"
#include "GameValidator.hpp"

class Room {
private:
    int id;
    int required_players;
    std::vector<Client*> players;
    bool game_started;

    // Logika gry
    std::vector<Tile> deck;     // Talia
    int current_turn_index;     // Kto teraz gra?
    std::vector<Tile> table;    // Klocki na stole

    // Backupy do cofania tury w razie błędu
    std::vector<Tile> backup_table;
    std::vector<Tile> backup_current_hand;

    int consecutive_passes; // Licznik pasów (do wykrycia remisu)
    
    void initDeck();
    void nextTurn();
    void endGame();

    void restoreState(Client* player);
    bool validateCommit(Client* sender, const std::string& data, std::vector<Tile>& out_new_table, std::vector<Tile>& out_used_from_hand);

public:
    Room(int room_id, int req_players);
    
    // Zarządzanie graczami
    bool addPlayer(Client* client);
    void removePlayer(Client* client);
    
    // Komunikacja
    void broadcast(std::string message, Client* exclude = nullptr);
    
    // Stan gry
    bool isFull() const;
    void start();
    int getId() const { return id; }
    const std::vector<Client*>& getPlayers() const { return players; }

    // Główna funkcja przetwarzająca wiadomości od graczy w trakcie gry
    void handleGameMessage(Client* sender, std::string msg);

    // Konkretne akcje
    void handleDraw(Client* sender);
    void handlePos(Client* sender, std::string data);
    void handlePull(Client* sender, std::string data);
    void handleCommit(Client* sender, std::string data);
};

#endif