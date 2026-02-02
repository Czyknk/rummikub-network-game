#ifndef GAME_VALIDATOR_HPP
#define GAME_VALIDATOR_HPP

#include "Shared.hpp"
#include <vector>
#include <algorithm>
#include <map>
#include <set>

class GameValidator {
public:
    struct ValidationResult {
        bool valid;
        std::string message;
        int points; // Zwracamy punkty (istotne przy otwarciu)
    };

    // Główna metoda walidująca
    static ValidationResult validateBoard(
        const std::vector<Tile>& old_flat_table,         // Stary stół
        const std::vector<std::vector<Tile>>& new_groups, // Nowy układ (podział na grupy)
        const std::vector<Tile>& player_hand,            // Ręka gracza (oryginalna, z backupu)
        bool has_initial_meld                            // Czy gracz ma już otwarcie?
    );

private:
    static bool isGroupValid(const std::vector<Tile>& group, int& out_points); 
    static bool isRunValid(std::vector<Tile> group, int& out_points); 
    
    static bool validateStructure(
        const std::vector<Tile>& old_table,
        const std::vector<std::vector<Tile>>& new_groups,
        bool has_initial_meld,
        std::string& err_msg
    );
};

#endif