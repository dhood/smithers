#include "messages.h"  


namespace smithers{



Json::Value create_dealt_hands_message(
        const std::vector<Hand>& hands,
        const std::vector<Player>& players,
        int dealer)
{
    int hand_number = 0;
    Json::Value players_json(Json::arrayValue);
    
    for (size_t i=0; i<players.size(); ++i){
        
        int deal_to_seat = (dealer + 1 + i) % players.size(); // start left of dealer

        if (!players[deal_to_seat].m_in_play){
            continue;
        }

        Json::Value player;
        player["name"] = players[deal_to_seat].m_name;
        player["chips"] = players[deal_to_seat].m_chips;
        player["hand"] << hands[hand_number];

        players_json.append(player);
        hand_number++;
    }

    Json::Value root;
    root["type"] = "DEALT_HANDS";
    root["pot"] = 0;
    root["players"] = players_json;

    return root;
}

Json::Value create_tournament_winner_message(const std::string& winner, int chips)
{
    Json::Value root;
    root["type"]="WINNER";
    root["name"]=winner;
    root["winnings"] = chips;

    return root;
}

Json::Value create_table_cards_message(const std::vector<Card>& cards, int pot)
{
    Json::Value card_vector(Json::arrayValue);
    for (std::vector<Card>::const_iterator c_it = cards.cbegin();
        c_it != cards.cend();
        c_it++){
        // ugly
        std::ostringstream c;
        c << *c_it;
        card_vector.append(c.str());
    } 

    Json::Value root;
    root["type"] = "DEALT_HANDS";
    root["pot"] = pot;
    root["cards"] = card_vector;

    return root;
}

}