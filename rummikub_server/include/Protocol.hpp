#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <string>

namespace Cmd {
    // --- KOMENDY OD KLIENTA (Requests) ---
    constexpr const char* JOIN      = "JOIN";           // JOIN <mode> <nick> Dołączenie do pokoju
    constexpr const char* DRAW      = "DRAW";           // DRAW Dobranie klocka
    constexpr const char* POS       = "POS";            // POS <TileID> <X> <Y> Położenie na stole / Przesunięcie
    constexpr const char* PULL      = "PULL";           // PULL <TileID> Zabranie ze stołu
    constexpr const char* COMMIT    = "COMMIT";         // COMMIT <Tile1ID> <Tile2ID>... | <Tile1ID>... Zatwierdzenie ruchu
    
    // --- KOMUNIKATY OD SERWERA (Responses/Events) ---
    constexpr const char* WELCOME   = "WELCOME_HAND";   // WELCOME_HAND <MY_ID> <Tile1> <Tile2> ... Klocki na start
    constexpr const char* START     = "GAME_START";     // GAME_START Start gry
    constexpr const char* TURN      = "TURN_INFO";      // TURN_INFO <ID> Czyja tura
    constexpr const char* TILE_NEW  = "TILE_DRAWN";     // TILE_DRAWN <TileID> Wynik dobrania (dla gracza)
    constexpr const char* OPP_DRAW  = "OPP_DRAWN";      // OPP_DRAWN <TileID> Wynik dobrania (dla innych)
    
    constexpr const char* COUNTS = "HAND_COUNTS"; // HAND_COUNTS c0 c1 c2 c3
    
    constexpr const char* BOARD     = "BOARD_UPDATE";   // BOARD_UPDATE <ID:X:Y:COLOR:VALUE> <tile_data2> ... Pełna synchronizacja (np. po resecie)
    constexpr const char* ERROR     = "ERROR";          // ERROR <msg> Błąd (z opisem)
    constexpr const char* GAME_OVER = "GAME_OVER";      // GAME_OVER <WINNER_ID | ID:SCORE ID:SCORE ...> Wyniki
}

// Funkcja pomocnicza - dzięki 'inline' może być w pliku .hpp bez błędu linkera
inline bool startsWith(const std::string& fullString, const std::string& prefix) {
    if (fullString.length() < prefix.length()) return false;
    return fullString.compare(0, prefix.length(), prefix) == 0;
}

#endif