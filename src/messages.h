#ifndef MESSAGES
#define MESSAGES

#include "player.h"
#include "card.h"

#include <json/json.h>




namespace smithers{


    Json::Value create_dealt_hands_message(
        const std::vector<Hand>& hands,
        const std::vector<Player>& players,
        int dealer
    );
    
    Json::Value create_tournament_winner_message(
        const std::string& winner,
        int pot
    );
    
    Json::Value create_table_cards_message(
        const std::vector<Card>& cards,
        int pot
    );
    
    Json::Value create_move_request(const Player& player, int pot, int last_bet);
    // Json::Value create_move_message(const Player& player, enum MoveType move, int amount);
    // Json::Value create_results_message(const std::vector<Result_t>& results);


}

#endif 