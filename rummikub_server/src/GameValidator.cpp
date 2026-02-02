#include "../include/GameValidator.hpp"
#include <iostream>

GameValidator::ValidationResult GameValidator::validateBoard(
    const std::vector<Tile>& old_flat_table,
    const std::vector<std::vector<Tile>>& new_groups,
    const std::vector<Tile>& player_hand,
    bool has_initial_meld
) {
    (void)player_hand; 

    // Walidacja Integralności - Czy wszystkie stare klocki wróciły na stół?
    std::map<int, bool> old_ids_found;
    for(const auto& t : old_flat_table) {
        old_ids_found[t.id] = false;
    }

    int tiles_from_hand_count = 0;
    int total_points_added = 0; 
    int group_idx = 0;
    
    // Przechodzenie przez wszystkie grupy
    for (const auto& group : new_groups) {
        int group_points = 0;
        
        // Sprawdzanie struktury grupy (Run/Group)
        int first_color = -1;
        bool multi_color = false;
        int non_joker_count = 0;

        // Analiza kolorów w grupie
        for(const auto& t : group) {
            if (t.value == 0) continue; 
            non_joker_count++;
            if (first_color == -1) first_color = t.color;
            else if (t.color != first_color) multi_color = true;
        }

        bool valid_logic = false;
        
        // Grupa składająca się wyłącznie z Jokerów
        if (non_joker_count == 0 && group.size() > 0) {
             std::cout << "[DEBUG VALIDATOR] Group " << group_idx << " consists only of Jokers.\n";
             return {false, "Group of only Jokers is invalid", 0};
        }
        // Sprawdzanie logiki grupy
        if (multi_color) {
            valid_logic = isGroupValid(group, group_points);
            if (!valid_logic) std::cout << "[DEBUG VALIDATOR] Group " << group_idx << " failed Set (Group) validation.\n";
        } else {
            valid_logic = isRunValid(group, group_points);
             if (!valid_logic) std::cout << "[DEBUG VALIDATOR] Group " << group_idx << " failed Run validation.\n";
        }
        // Jeśli grupa nie przeszła żadnej walidacji
        if (!valid_logic) {
            return {false, "Invalid group found (logic error in group " + std::to_string(group_idx) + ")", 0};
        }

        // Sprawdzanie pochodzenia klocków (z ręki czy ze stołu)
        bool has_old_tiles = false;
        bool has_new_tiles = false;

        for (const auto& t : group) {
            if (old_ids_found.count(t.id)) {
                old_ids_found[t.id] = true; 
                has_old_tiles = true;
            } else {
                has_new_tiles = true;
                tiles_from_hand_count++;
            }
        }

        // Zakaz mieszania klocków ze stołu i z ręki przy otwarciu
        if (!has_initial_meld) {
            if (has_old_tiles && has_new_tiles) {
                std::cout << "[DEBUG VALIDATOR] Initial Meld Violation: Group " << group_idx << " mixes hand and table tiles.\n";
                return {false, "Initial Meld: Cannot use/modify tiles from table (mixed group detected)", 0};
            }
        }

        // Punkty liczymy tylko z grup całkowicie nowych (czystych)
        if (has_new_tiles && !has_old_tiles) {
            total_points_added += group_points;
        }

        group_idx++;
    }

    // Czy wszystkie stare klocki wróciły na stół?
    for (auto const& pair : old_ids_found) {
        if (!pair.second) {
            std::cout << "[DEBUG VALIDATOR] Missing old tile ID: " << pair.first << "\n";
            return {false, "Missing tile from old table (Cannot take back/swap)", 0};
        }
    }

    // Czy gracz cokolwiek dołożył?
    if (tiles_from_hand_count == 0) {
        std::cout << "[DEBUG VALIDATOR] No tiles played from hand.\n";
        return {false, "Must play at least one tile", 0};
    }

    // Zasady Otwarcia (30 pkt)
    if (!has_initial_meld) {
        if (total_points_added < 30) {
            std::cout << "[DEBUG VALIDATOR] Initial meld points: " << total_points_added << " (required 30).\n";
            return {false, "Initial Meld requires 30 points from hand (pure sets)", 0};
        }
    }

    return {true, "OK", total_points_added};
}

bool GameValidator::isGroupValid(const std::vector<Tile>& group, int& out_points) {
    if (group.size() < 3 || group.size() > 4) return false;

    int value = -1;
    std::set<int> colors;

    for (const auto& t : group) {
        if (t.value == 0) { // Joker
            // Joker w grupie przyjmuje wartość pozostałych
        } else {
            if (value == -1) value = t.value;
            else if (t.value != value) return false;
            
            if (colors.count(t.color)) return false; 
            colors.insert(t.color);
        }
    }
    
    // Jeśli same jokery
    if (value == -1) return false; 

    out_points = group.size() * value;
    return true;
}

bool GameValidator::isRunValid(std::vector<Tile> group, int& out_points) {
    if (group.size() < 3) return false;

    int anchor_val = -1;
    int anchor_idx = -1;
    int color = -1;

    // Znajdowanie (pierwszy nie-joker)
    for(size_t i = 0; i < group.size(); ++i) {
        if (group[i].value != 0) {
            anchor_val = group[i].value;
            anchor_idx = i;
            color = group[i].color;
            break;
        }
    }

    // Jeśli same jokery
    if (anchor_val == -1) return false;

    out_points = 0;

    // Sprawdzanie ciągłości w obie strony od anchora
    for(size_t i = 0; i < group.size(); ++i) {
        int expected_val = anchor_val - (static_cast<int>(anchor_idx) - static_cast<int>(i));

        if (expected_val < 1 || expected_val > 13) return false;

        if (group[i].value == 0) {
            out_points += expected_val;
        } else {
            if (group[i].value != expected_val) return false;
            if (group[i].color != color) return false;
            out_points += group[i].value;
        }
    }

    return true;
}