#include "../include/Room.hpp"
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <chrono>

// Helper do rozdzielania stringów
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

Room::Room(int room_id, int req_players) 
    : id(room_id), required_players(req_players), game_started(false), consecutive_passes(0) {}

bool Room::addPlayer(Client* client) {
    if (isFull()) return false;
    
    players.push_back(client); // Dodanie do wektora graczy
    client->room_id = id;
    
    // Powiadomienie wszystkich o nowym graczu
    std::string msg = "Player joined. " + std::to_string(players.size()) 
                      + "/" + std::to_string(required_players) + "\n";
    broadcast(msg);
    
    return true;
}

void Room::removePlayer(Client* client) {
    auto it = std::remove(players.begin(), players.end(), client); // usuń gracza z wektora
    players.erase(it, players.end());
    
    client->room_id = -1;
    broadcast("Player left the room.\n");
}

void Room::broadcast(std::string message, Client* exclude) { // wysyłanie do wszystkich oprócz exclude
    for (Client* p : players) {
        if (p == exclude) continue;
        send(p->fd, message.c_str(), message.length(), 0);
    }
}

bool Room::isFull() const {
    return (int)players.size() >= required_players;
}

void Room::initDeck() {
    deck.clear();
    int id_counter = 1;

    // 2 zestawy klocków (1-13 w 4 kolorach)
    for (int set = 0; set < 2; set++) {
        for (int color = 0; color < 4; color++) {
            for (int val = 1; val <= 13; val++) {
                deck.push_back({id_counter++, val, color});
            }
        }
    }
    
    // 2 Jokery
    deck.push_back({id_counter++, 0, JOKER});
    deck.push_back({id_counter++, 0, JOKER});

    // Tasowanie
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(deck.begin(), deck.end(), std::default_random_engine(seed));
    
    std::cout << "\t[DEBUG] [ROOM_" << id << "] Deck initialized with " << deck.size() << " tiles." << std::endl; // #DEBUG
}

void Room::start() {
    if (game_started) return;
    game_started = true;
    consecutive_passes = 0; // Reset licznika pasów na start gry
    table.clear();
    
    initDeck();
    current_turn_index = 0;

    // Rozdanie kart
    for (Client* player : players) {
        player->hand.clear();
        for (int i = 0; i < INITIAL_HAND_SIZE; i++) {
            if (deck.empty()) break;
            player->hand.push_back(deck.back());
            deck.pop_back();
        }
    }

    // GAME_START 0 NickA 1 NickB ...
    std::stringstream ss_start;
    ss_start << Cmd::START;
    
    int idx = 0;
    for (Client* p : players) {
        ss_start << " " << idx << " " << p->nick;
        idx++;
    }
    ss_start << "\n";

    std::cout << "\t[DEBUG] [ROOM_" << id << "] GAME_START message: " << ss_start.str(); // #DEBUG

    // wysłanie START do wszystkich
    broadcast(ss_start.str());

    // wysłanie WELCOME_HAND z ID gracza
    int p_idx = 0;
    for (Client* player : players) {
        // Format: WELCOME_HAND <MY_ID> <t1> <t2>...
        std::string msg = std::string(Cmd::WELCOME) + " " + std::to_string(p_idx) + " ";
        for (const auto& tile : player->hand) {
            msg += tile.toString() + " ";
        }
        msg += "\n";
        send(player->fd, msg.c_str(), msg.length(), 0);

        std::cout << "\t[DEBUG] [ROOM_" << id << "] Sent welcome hand to Player " << p_idx << ": " << msg; // #DEBUG
        p_idx++;
    }
     
    // Backup i pierwsza tura
    Client* first = players[current_turn_index];
    backup_table = table;
    backup_current_hand = first->hand;
    
    // Info kto zaczyna
    broadcast(std::string(Cmd::TURN) + " " + std::to_string(current_turn_index) + "\n");
}

void Room::handleGameMessage(Client* sender, std::string msg) {
    if (!game_started) return;

    // Sprawdzenie czy to tura tego gracza
    if (players[current_turn_index] != sender) {
        std::string err = std::string(Cmd::ERROR) + " Not your turn\n";
        send(sender->fd, err.c_str(), err.length(), 0);
        return;
    }

    // Obsługa komend gry
    if (msg == Cmd::DRAW) {
        handleDraw(sender);
    } 
    else if (startsWith(msg, Cmd::POS)) {
        handlePos(sender, msg);
    }
    else if (startsWith(msg, Cmd::PULL)) {
        handlePull(sender, msg);
    }
    else if (startsWith(msg, Cmd::COMMIT)) {
        handleCommit(sender, msg);
    }
}

void Room::handleDraw(Client* sender) {
    // Jeżeli dobieramy, to znaczy że pasujemy - cofnięcie zmian
    restoreState(sender);

    if (deck.empty()) {
        consecutive_passes++;
        if (consecutive_passes >= (int)players.size()) { // brak ruchów u wszystkich, koniec gry
            endGame();
            return;
        }
        nextTurn();
        return;
    }

    // Dobranie klocka
    Tile t = deck.back(); 
    deck.pop_back();
    sender->hand.push_back(t);
    
    // Reset licznika pasów 
    consecutive_passes = 0;

    std::string msg = std::string(Cmd::TILE_NEW) + " " + t.toString() + "\n";
    send(sender->fd, msg.c_str(), msg.length(), 0);
    std::cout << "\t[DEBUG] [ROOM_" << id << "] Sent new tile to C_" << sender->fd << ": " << msg; // #DEBUG
    
    broadcast(std::string(Cmd::OPP_DRAW) + "\n", sender);
    
    nextTurn();
}

void Room::handlePos(Client* sender, std::string data) {
    // pozycja: POS <TileID> <X> <Y>
    std::stringstream ss(data);
    std::string cmd, x_str, y_str;
    int id, x, y;
    ss >> cmd >> id >> x_str >> y_str; 

    try {
        x = std::stoi(x_str);
        y = std::stoi(y_str);
    } catch (...) { return; }

    Tile* found_tile = nullptr;

    // Szukanie klocka na stole
    for (auto& t : table) {
        if (t.id == id) { 
            found_tile = &t; 
            t.x = x;
            t.y = y;
            break; 
        }
    }
    // Szukanie klocka na ręce gracza - nowy klocek
    if (!found_tile) {
        for (auto& t : sender->hand) {
            if (t.id == id) { 
                found_tile = &t; 
                t.x = x;
                t.y = y;
                break; 
            }
        }
    }

    if (!found_tile) return; // błędne ID

    // Broadcast do innych graczy
    std::string msg_out = std::string(Cmd::POS) + " " + std::to_string(id) + " " 
                          + std::to_string(x) + " " + std::to_string(y) + " "
                          + std::to_string(found_tile->color) + " " 
                          + std::to_string(found_tile->value) + "\n";

    broadcast(msg_out, sender);
}

void Room::handlePull(Client* sender, std::string data) {
    // Format: PULL <id>
    std::stringstream ss(data);
    std::string cmd;
    int id;
    ss >> cmd >> id;

    // Nie wolno zabrać klocka, który był na stole na początku tury (backup_table)
    for (const auto& t : backup_table) {
        if (t.id == id) {
            std::string err = std::string(Cmd::ERROR) + " Cannot pull committed tile\n";
            send(sender->fd, err.c_str(), err.length(), 0);
            
            return;
        }
    }

    // Komunikat do wszystkich o zabraniu klocka
    std::string msg = std::string(Cmd::PULL) + " " + std::to_string(id) + "\n";
    broadcast(msg, sender);
}

void Room::handleCommit(Client* sender, std::string data) {
    // Format: COMMIT <data>
    std::vector<Tile> new_table_state;
    std::vector<Tile> used_from_hand;

    // Walidacja
    if (!validateCommit(sender, data, new_table_state, used_from_hand)) {
        // BŁĄD WALIDACJI
        restoreState(sender);
        
        // Kara: Dobierz klocek (jeśli jest)
        if (!deck.empty()) {
            Tile t = deck.back();
            deck.pop_back();
            sender->hand.push_back(t);
            
            // Wysłanie info o dobraniu klocka
            std::string msg = std::string(Cmd::TILE_NEW) + " " + t.toString() + "\n";
            send(sender->fd, msg.c_str(), msg.length(), 0);
            broadcast(std::string(Cmd::OPP_DRAW) + "\n", sender);
        }
        
        consecutive_passes++; // Błędny ruch to też brak postępu
        nextTurn();
        return;
    }

    // SUKCES WALIDACJI
    table = new_table_state; // Aktualizacja stanu stołu
    
    // Usuwanie klocków z ręki gracza
    for (const auto& used : used_from_hand) {
        auto it = std::remove_if(sender->hand.begin(), sender->hand.end(), 
            [&](const Tile& t){ return t.id == used.id; });
        sender->hand.erase(it, sender->hand.end());
    }

    // Sprawdzenie czy gracz wygrał
    if (sender->hand.empty()) {
        endGame();
        return;
    }

    // Reset licznika pasów jeśli coś zostało użyte z ręki
    if (!used_from_hand.empty()) consecutive_passes = 0;
    else consecutive_passes++;

    nextTurn();
}

bool Room::validateCommit(Client* sender, const std::string& data, std::vector<Tile>& out_new_table, std::vector<Tile>& out_used_from_hand) {
    // Format: COMMIT <group1>|<group2>|... gdzie group: id,id,id,...
    std::string content = data.substr(7); 
    if (content.empty()) return false;

    std::vector<std::vector<Tile>> new_groups;
    std::vector<int> all_ids_check; // Do szybkiego sprawdzenia dostępności

    // Budowanie mapy wszystkich dostępnych klocków (Stół + Ręka)
    std::map<int, Tile> source_tiles;
    for (const auto& t : backup_table) source_tiles[t.id] = t;
    for (const auto& t : backup_current_hand) source_tiles[t.id] = t;

    auto groups_str = split(content, '|');

    for (const auto& g_str : groups_str) {
        std::vector<Tile> current_group;
        auto ids = split(g_str, ',');
        
        for (const auto& id_str : ids) {
            int id;
            try { id = std::stoi(id_str); } catch(...) { return false; }
            
            if (source_tiles.find(id) == source_tiles.end()) {
                return false; // Próba użycia klocka z kosmosu
            }
            
            // klocek z backupu
            Tile t = source_tiles[id];

            // Ustawianie pozycji X/Y klocka (jeśli był przesuwany)
            bool coords_found = false;

            // Szukanie w aktualnym stole
            for (const auto& live_t : table) {
                if (live_t.id == id) {
                    t.x = live_t.x;
                    t.y = live_t.y;
                    coords_found = true;
                    break;
                }
            }

            // Szukanie w aktualnej ręce (jeśli właśnie został wyłożony)
            if (!coords_found) {
                for (const auto& live_t : sender->hand) {
                    if (live_t.id == id) {
                        t.x = live_t.x;
                        t.y = live_t.y;
                        coords_found = true;
                        break;
                    }
                }
            }

            // Dodanie do grupy i listy użytych
            current_group.push_back(t);
            all_ids_check.push_back(id);
        }
        new_groups.push_back(current_group);
    }

    // WYWOŁANIE WALIDATORA
    auto result = GameValidator::validateBoard(
        backup_table, 
        new_groups, 
        sender->hand, // Oryginalna ręka z backupu
        sender->has_initial_meld
    );

    // Sprawdzenie wyniku walidacji
    if (!result.valid) {
        std::string err = std::string(Cmd::ERROR) + " " + result.message + "\n";
        send(sender->fd, err.c_str(), err.length(), 0);
        return false;
    }

    // Aktualizacja statusu otwarcia gracza
    if (!sender->has_initial_meld) {
        sender->has_initial_meld = true; // Zaliczone
    }

    // Budowanie out_new_table i out_used_from_hand do zwrotu
    out_new_table.clear();
    for(const auto& g : new_groups) {
        for(const auto& t : g) out_new_table.push_back(t);
    }
    
    // Klocki użyte z ręki
    out_used_from_hand.clear();
    for (int id : all_ids_check) {
        // Jeśli nie było go w backup_table, to jest z ręki
        bool was_on_table = false;
        for(const auto& bt : backup_table) {
            if (bt.id == id) { was_on_table = true; break; }
        }
        if (!was_on_table) {
            out_used_from_hand.push_back(source_tiles[id]);
        }
    }

    return true;
}

void Room::restoreState(Client* player) {
    table = backup_table; // Przywrócenie stołu
    player->hand = backup_current_hand; // Przywrócenie ręki gracza
    
    // BOARD_UPDATE do wszystkich (wszyscy widzieli zły ruch)
    std::string msg = std::string(Cmd::BOARD) + " ";
    for (const auto& t : table) {
        // Format: id:x:y:col:val
        msg += t.toFullString() + " "; 
    }
    msg += "\n";
    // std::cout << "\t[DEBUG] BOARD_UPDATE message: " << msg; // #DEBUG
    broadcast(msg);
    
    // Wysłanie do gracza jego ręki (WELCOME_HAND)
    std::string hand_msg = std::string(Cmd::WELCOME) + " " + std::to_string(current_turn_index) + " "; // ID nie ma znaczenia przy resecie, ale format musi być zachowany
    for (const auto& t : player->hand) hand_msg += t.toString() + " ";
    hand_msg += "\n";
    send(player->fd, hand_msg.c_str(), hand_msg.length(), 0);
    std::cout << "\t[DEBUG] [ROOM_" << id << "] Sent restored hand to Player " << player->fd << ": " << hand_msg; // #DEBUG

    // wysłanie liczb kart w rękach wszystkich graczy (HAND_COUNTS)
    std::string counts_msg = std::string(Cmd::COUNTS);
    for (Client* c : players) {
        counts_msg += " " + std::to_string(c->hand.size());
    }
    counts_msg += "\n";
    std::cout << "\t[DEBUG] [ROOM_" << id << "] Sent HAND_COUNTS message: " << counts_msg; // #DEBUG
    broadcast(counts_msg);
}

void Room::nextTurn() {
    current_turn_index = (current_turn_index + 1) % players.size(); // Następny gracz w kolejce 
    Client* current_player = players[current_turn_index];

    // Backup stanu przed turą
    backup_table = table; 
    backup_current_hand = current_player->hand;

    //stan stołu #DEBUG
    std::string board_msg = "";
    for (const auto& t : table) {
        board_msg += t.toFullString() + " ";
    }
    board_msg += "\n";
    std::cout << "\t[DEBUG] [ROOM_" << id << "] Current Board State: " << board_msg;
    std::cout << "\t[DEBUG] [ROOM_" << id << "] Turn passed to player index: " << current_turn_index << std::endl;

    // Info dla wszystkich
    broadcast(std::string(Cmd::TURN) + " " + std::to_string(current_turn_index) + "\n");
}

void Room::endGame() {
    // Format: GAME_OVER <WINNER_ID | ID:SCORE ID:SCORE ...>
    std::stringstream ss; 
    ss << Cmd::GAME_OVER << " ";
    
    int winner_idx = -1;
    int min_points = 10000;

    // Znajdowanie zwycięzcy (najmniej punktów karnych)
    for (int i=0; i < (int)players.size(); i++) {
        int ptr = calculateHandPoints(players[i]->hand);
        if (ptr < min_points) {
            min_points = ptr;
            winner_idx = i;
        }
    }

    ss << winner_idx << " |";

    // Punkty karne wszystkich graczy
    for (int i=0; i < (int)players.size(); i++) {
        int penalty = calculateHandPoints(players[i]->hand);
        ss << " " << i << ":" << penalty;
    }
    ss << "\n";

    // Wysłanie komunikatu o zakończeniu gry i wynikach
    broadcast(ss.str());
    std::cout << "\t[DEBUG] [ROOM_" << id << "] Game Over message: " << ss.str();
    
    game_started = false; 
    //usunięcie pokoju
    for (Client* p : players) {
        p->room_id = -1; // Wracają do lobby
    }
    players.clear();
}