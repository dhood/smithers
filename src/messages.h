#ifndef MESSAGES
#define MESSAGES

#include "player.h"
#include "card.h"

#include <json/json.h>




namespace smithers{

    typedef struct r {
            std::string score;
            std::string hand;
            size_t player_index;
            int winnings;
            int amount_bet;
    } Result_t;
    
    enum MoveType {
            ERROR = -1,
            RAISE,
            CALL,
            FOLD,
            ALL_IN,

    };
 
    Json::Value create_registered_message( const Player& player );

    Json::Value create_tournament_start_message( const std::vector<Player>& players);

    Json::Value create_dealt_hands_message( const std::vector<Hand>& hands,
                                            const std::vector<Player>& players,
                                            int dealer );

    Json::Value create_table_cards_message( const std::vector<Card>& cards, int pot );

    Json::Value create_move_request( const Player& player, int pot, int min_raise, int last_bet );
    
    Json::Value create_tournament_winner_message( const std::string& winner, int pot);

    Json::Value create_move_message(const Player& player, enum MoveType move, int amount, bool is_blind);
    
    Json::Value create_results_message(const std::vector<Result_t>& results,
                                       const std::vector<Player>& players );

    Json::Value create_broke_message(const std::vector<std::string>& broke_names);

    Json::Value create_ping_message();

    Json::Value create_shutdown_message();



}

#endif 