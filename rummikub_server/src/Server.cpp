#include "../include/Server.hpp"

GameServer::GameServer(int port_num) : port(port_num), next_room_id(1) {
    SocketFD = -1;
}

GameServer::~GameServer() {
    //czyszczenie
    for (auto const& pair : clients) {
        int fd = pair.first;
        Client* client = pair.second;
        
        close(fd);
        delete client;
    }

    clients.clear();

    if (SocketFD != -1) close(SocketFD);
}

// Ustawienie non-blocking na danym fd
void GameServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void GameServer::run() {
    // Inicjalizacja serwera
    struct sockaddr_in sa;
    SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (SocketFD == -1) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    // Opcja, żeby nie czekać na zwolnienie portu po restarcie serwera
    int yes = 1;
    setsockopt(SocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY); // nasłuchiwanie na wszystkich interfejsach

    // Bind
    if (bind(SocketFD, (struct sockaddr *)&sa, sizeof sa) == -1) {
        perror("bind failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    // NONBLOCK dla socketa nasłuchującego
    setNonBlocking(SocketFD);

    if (listen(SocketFD, 10) == -1) {
        perror("listen failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja poll_set 
    struct pollfd initial_fd;
    initial_fd.fd = SocketFD;
    initial_fd.events = POLLIN; // Nasłuchiwanie na dane do odczytu
    poll_set.push_back(initial_fd);

    std::cout << "\t[DEBUG] Server listening on port " << port << std::endl;

    bool compressPollArr = false; // do usówania zamkniętych fd

    // Główna pętla (nieskończona)
    for (;;) {
        // Wywołanie poll z timeoutem do 1 sekundy
        int n = poll(poll_set.data(), poll_set.size(), POLL_TIMEOUT);

        if (n < 0) {
            perror("poll fail");
            break;
        }

        if (n == 0) {
            // Timeout, brak zdarzeń
            continue; 
        }

        int currentSize = poll_set.size();

        // Przetwarzanie zdarzeń
        for (int i = 0; i < currentSize; i++) {
            if (poll_set[i].revents == 0) continue; // fd bez zdarzeń

            if (poll_set[i].revents & POLLIN) { 
                
                if (poll_set[i].fd == SocketFD) {
                    // NOWE POŁĄCZENIE
                    handleNewConnection();
                } else {
                    // DANE OD KLIENTA
                    handleClientData(i);
                    if (poll_set[i].fd == -1) compressPollArr = true;
                }
            } 
            else if (poll_set[i].revents & (POLLHUP | POLLERR)) {
                // Błąd lub rozłączenie
                std::cout << "\t[DEBUG] Error on fd " << poll_set[i].fd << std::endl;
                close(poll_set[i].fd);
                poll_set[i].fd = -1;
                compressPollArr = true;
            }
        }

        // Kompresja tablicy (usuwanie rozłączonych)
        if (compressPollArr) {
            compressPollArr = false;
            
            auto it = std::remove_if(poll_set.begin(), poll_set.end(), 
                [](const struct pollfd& p) { return p.fd == -1; });
            
            // Usówanie mapy w removeClient
            // czyszczenie wektora
            poll_set.erase(it, poll_set.end());
        }
    }
}

void GameServer::handleNewConnection() {
    struct sockaddr_in ca;
    socklen_t ca_size = sizeof(ca);
    
    // Aceptacja nowego połączenia
    int ConnectFD = accept(SocketFD, (struct sockaddr *)&ca, &ca_size);
    
    if (ConnectFD < 0) {
        if (errno != EWOULDBLOCK) perror("accept failed");
        return;
    }

    // Ustawienie non-blocking
    setNonBlocking(ConnectFD);

    std::cout << "\t[DEBUG] New connection from " << inet_ntoa(ca.sin_addr) << ":" << ntohs(ca.sin_port) 
              << " (fd: " << ConnectFD << ")\n";

    // Dodanie do poll_set
    struct pollfd new_pfd;
    new_pfd.fd = ConnectFD;
    new_pfd.events = POLLIN;
    poll_set.push_back(new_pfd);

    // Tworzenie nowego klienta
    // Dodanie do mapy klientów
    clients[ConnectFD] = new Client(ConnectFD, inet_ntoa(ca.sin_addr));
}

void GameServer::handleClientData(int index) {
    char buffer[BUFFER_SIZE];
    int fd = poll_set[index].fd;
    Client* client = clients[fd];

    // Odczyt danych
    int n = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (n <= 0) {
        if (n == 0 || errno != EWOULDBLOCK) {
            removeClient(index); // Usunięcie klienta z powodu rozłączenia lub błędu
        }
        return;
    }

    buffer[n] = '\0';
    std::string data(buffer);
    
    // usuwanie znaków nowej linii na końcu
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r')) {
        data.pop_back();
    }

    std::cout << "[RECV] C_" << fd << ": " << data << std::endl;

    // Przekazanie do procesora komend
    processCommand(client, data);
}

void GameServer::processCommand(Client* client, std::string cmd) {
    std::stringstream ss(cmd);
    std::string junk, mode_str, nick_str;
    ss >> junk >> mode_str >> nick_str; // JOIN, mode, nick

    // Sprawdzenie czy klient jest już w aktywnej grze
    if (client->room_id != -1) {
        std::string msg = "You are in room " + std::to_string(client->room_id) + ". ignoring lobby cmds.\n";
        if (rooms.count(client->room_id)) {
            // Przekaż do pokoju
            rooms[client->room_id]->handleGameMessage(client, cmd);
        }
        return;
    }

    // Ustaw nick
    if (nick_str.empty()) {
            client->nick = "Player"; // Domyślny
        } else {
            client->nick = nick_str;
        }

    // Przetwarzanie komend lobby
    if (startsWith(cmd, std::string(Cmd::JOIN) + " ")) { // Jeśli zaczyna się od "JOIN "
        try {
            int mode = std::stoi(cmd.substr(5));
            if (mode < 2 || mode > 4) throw std::invalid_argument("Invalid mode");
            addToLobby(client, mode); // Dodanie do odpowiedniej kolejki
        } catch (...) {
            std::string err = std::string(Cmd::ERROR) + ": Usage: JOIN <2|3|4>\n";
            send(client->fd, err.c_str(), err.length(), 0);
        }
    } else {
        std::string msg = "Unknown command. Try: JOIN 2\n";
        send(client->fd, msg.c_str(), msg.length(), 0);
    }
}

void GameServer::removeFromVector(std::vector<Client*>& vec, Client* target) {
    auto it = std::remove(vec.begin(), vec.end(), target);
    if (it != vec.end()) {
        vec.erase(it, vec.end());
    }
}

void GameServer::addToLobby(Client* client, int mode) {
    // Czyszczenie z innych kolejek
    removeFromVector(queue_2, client);
    removeFromVector(queue_3, client);
    removeFromVector(queue_4, client);

    // wskaznik do odpowiedniej kolejki
    std::vector<Client*>* target_queue = nullptr;
    if (mode == 2) target_queue = &queue_2;
    else if (mode == 3) target_queue = &queue_3;
    else if (mode == 4) target_queue = &queue_4;

    if (!target_queue) return; // Nieprawidłowy tryb

    // Sprawdzenie czy już jest w kolejce
    for (auto c : *target_queue) {
        if (c == client) return; 
    }

    // Dodanie do kolejki
    target_queue->push_back(client);

    // Potwierdzenie do klienta
    std::string msg = "Joined queue for " + std::to_string(mode) + " players. Waiting...\n";
    send(client->fd, msg.c_str(), msg.length(), 0);

    std::cout << "\t[DEBUG] C_" << client->fd << ": joined queue " << mode 
              << ". Size: " << target_queue->size() << std::endl;

    // Sprawdź czy można utworzyć pokój
    if ((int)target_queue->size() >= mode) {
        createRoom(*target_queue, mode);
    }
}

void GameServer::createRoom(std::vector<Client*>& queue, int mode) {
    int id = next_room_id++; // inkrementacja po przydzieleniu
    Room* new_room = new Room(id, mode); // tworzenie pokoju
    rooms[id] = new_room;

    std::cout << "\t[DEBUG] Creating Room " << id << " for " << mode << " players." << std::endl;

    // Przeniesienie graczy z kolejki do pokoju
    for (int i = 0; i < mode; i++) {
        if (queue.empty()) break;

        Client* c = queue[0]; 
        queue.erase(queue.begin()); // usunięcie z kolejki FIFO

        std::cout << "\t[DEBUG] C_" << c->fd << ": deleted from queue " << mode << std::endl;

        new_room->addPlayer(c);
    }

    new_room->start(); // Rozpoczęcie gry
}

void GameServer::removeClient(int index) {
    int fd = poll_set[index].fd;
    
    // Sprawdzenie czy klient istnieje
    if (clients.find(fd) == clients.end()) {
        close(fd);
        poll_set[index].fd = -1;
        return;
    }

    Client* leaver = clients[fd];

    std::cout << "\t[DEBUG] C_" << fd << ": disconnected." << std::endl;

    // Gracz był w trakcie gry:
    if (leaver->room_id != -1) {
        int r_id = leaver->room_id;
        
        if (rooms.count(r_id)) {
            Room* room = rooms[r_id];
            
            // powiadomienie pozostałych graczy o rozłączeniu
            std::string err = std::string(Cmd::ERROR) + " Opponent disconnected. Game over.\n";
            std::string msg = std::string(Cmd::GAME_OVER) + "\n";
            
            for (Client* survivor : room->getPlayers()) {
                if (survivor != leaver) {
                    send(survivor->fd, err.c_str(), err.length(), 0);
                    send(survivor->fd, msg.c_str(), msg.length(), 0);

                    survivor->room_id = -1; // Powrót do lobby
                }
            }
            
            // Usunięcie pokoju
            std::cout << "\t[DEBUG] Destroying Room " << r_id << " due to disconnect." << std::endl;
            delete room;
            rooms.erase(r_id); // usunięcie z mapy
        }
    } 
    // Gracz był w lobby:
    else {
        // Usunięcie z kolejek lobby
        removeFromVector(queue_2, leaver);
        removeFromVector(queue_3, leaver);
        removeFromVector(queue_4, leaver);

        std::cout << "\t[DEBUG] Removed client C_" << fd << " from lobby queues." << std::endl;
    }

    // Sprzątanie
    delete leaver;
    clients.erase(fd);
    close(fd); // zamknięcie gniazda

    poll_set[index].fd = -1; // Oznaczenie do usunięcia przy kompresji poll_set
}