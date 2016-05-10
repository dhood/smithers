#include "smithers.h"

#include <zmq.hpp>
#include <m2pp.hpp>
#include <json/json.h>

#include <iostream>
#include <sstream>
#include <random>
#include <algorithm> 


// #if (defined (WIN32))
// #include <zhelpers.hpp>
// #endif


namespace {

std::string gen_random(const size_t len) {
    char s[len];
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::random_device gen;

    for (size_t i = 0; i < len; ++i) {
        s[i] = alphanum[gen() % (sizeof(alphanum) - 1)];
    }
    s[len] = 0; // null
    
    return std::string(s);
}

void log_request(const m2pp::request& req) {
    std::ostringstream log_request;

    log_request << "<pre>" << std::endl;
    log_request << "SENDER: " << req.sender << std::endl;
    log_request << "IDENT: " << req.conn_id << std::endl;
    log_request << "PATH: " << req.path << std::endl;
    log_request << "BODY: " << req.body << std::endl;
    
    for (std::vector<m2pp::header>::const_iterator it=req.headers.cbegin();it!=req.headers.cend();it++) {
        log_request << "HEADER: " << it->first << ": " << it->second << std::endl;
    }

    log_request << "</pre>" << std::endl;

    std::cout << log_request.str();
}

bool is_dealer(const smithers::Player& p){
    return p.m_is_dealer;
}

} // close anon namespace

namespace smithers{

Smithers::Smithers():
    m_zmq_context(1),
    m_publisher(m_zmq_context, ZMQ_PUB){
        m_publisher.bind("tcp://127.0.0.1:9950");
}


void Smithers::await_registered_players(const int max_players){
    std::cout << "await_registered_players().." << std::endl;

    m2pp::connection conn("UUID_1", "tcp://127.0.0.1:9997", "tcp://127.0.0.1:9996");
    
    int seat = 1;
    while (true){
        m2pp::request req = conn.recv();

        if (req.disconnect) {
            std::cout << "== disconnect ==" << std::endl;
            continue;
        }

        log_request(req);

        std::ostringstream name;
        name << "Player" << seat;
        
        Player new_player(name.str(), gen_random(20), seat);
        
        std::ostringstream player_info;
        player_info << "{ "
                    << "\"name\":\"" << new_player.m_name << "\","
                    << "\"cash\":10000,"
                    << "\"key\":\""<< new_player.m_hash_key<<"\""
                    << " }" << std::endl;

        conn.reply_http(req, player_info.str());
        m_players.push_back(new_player);
        if (seat < max_players){
            seat++;
        } else {
            break;
        }

    }
    m_players[0].m_is_dealer = true;

}

void Smithers::publish_to_all(const std::string& message){
    std::cout << "ALL: " << message << std::endl;
    zmq::message_t zmq_message(message.begin(), message.end());
    m_publisher.send(zmq_message);
}

void Smithers::publish_to_all(const Json::Value& json){
    std::ostringstream message;
    message << json;
    publish_to_all(message.str());
}

void Smithers::play_game(){
    std::cout << "play_game" << std::endl;
    
    Game new_game;
    std::vector<Hand> hands = new_game.deal_hands(m_players.size()); // need to eject players
    Json::Value dealt_hands = create_dealt_hands_message(hands);
    // add blinds, set dealer
    publish_to_all(dealt_hands);

    play_betting_round(3);

    new_game.deal_flop();
    Json::Value flop = create_table_cards_message(new_game.get_table());
    // add pot
    publish_to_all(flop);

    play_betting_round(1);

    new_game.deal_river();
    Json::Value river = create_table_cards_message(new_game.get_table());
    // add pot
    publish_to_all(river);

    play_betting_round(1);
    
    new_game.deal_turn();
    Json::Value turn = create_table_cards_message(new_game.get_table());
    // add pot
    publish_to_all(turn);

    play_betting_round(1);

    reset_and_move_dealer_to_next_player();

}

Json::Value Smithers::create_dealt_hands_message(const std::vector<Hand>& hands){
    
    Json::Value players(Json::arrayValue);
    for (size_t i=0; i<m_players.size(); ++i){
        if (!m_players[i].m_in_play){
            continue;
        }
        Json::Value player;
        player["name"] = m_players[i].m_name;
        player["chips"] = m_players[i].m_chips;
        player["hand"] << hands[i];

        players.append(player);
        
    }

    Json::Value root;
    root["type"] = "DEALT_HANDS";
    root["pot"] = 0;
    root["players"] = players;

    return root;
}

Json::Value Smithers::create_table_cards_message(const std::vector<Card>& cards){
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
    root["pot"] = 0;
    root["cards"] = card_vector;

    return root;
}

Json::Value Smithers::create_move_request(const std::string& name){
    Json::Value root;
    root["type"] = "MOVE_REQUEST";
    root["pot"] = 0;
    root["name"] = name;

    return root;
}

void Smithers::listen_and_pull_from_queue(){
    m2pp::connection conn("ID", "tcp://127.0.0.1:9900", "tcp://127.0.0.1:9901");
    while (true){
        m2pp::request req = conn.recv();


        if (req.disconnect) {
            std::cout << "== disconnect ==" << std::endl;
            continue;
        } else {
            std::cout << req.body << std::endl;
            conn.reply_http(req, "VALID");
            break;
        }
    }

}

int Smithers::get_dealer(){
    std::vector<Player>::iterator it = std::find_if(m_players.begin(), m_players.end(), is_dealer);
    return it - m_players.begin(); 
}

int Smithers::get_next_to_play(int seat){
    int next = (seat + 1) % m_players.size();
    if (m_players[next].m_in_play && m_players[next].m_in_play_this_round){
        return next;
    }
    else {
        return get_next_to_play(next);
    }
}

void Smithers::reset_and_move_dealer_to_next_player(){
    int dealer = get_dealer();
    for (size_t i=0; i<m_players.size(); ++i){
        m_players[i].m_in_play_this_round = true;
    }
    int next_dealer = get_next_to_play(dealer);

    std::cout<< "OLD DEALER: " << dealer << " "<< m_players[dealer].m_name<<std::endl;
    std::cout<< "NEW DEALER: " << next_dealer << " " <<m_players[next_dealer].m_name<<std::endl;
    m_players[dealer].m_is_dealer = false;
    m_players[next_dealer].m_is_dealer = true;

};

void Smithers::play_betting_round(int first_to_bet){
    int to_play_index = get_dealer();
    for (int i=0; i<first_to_bet; i++){
        to_play_index = get_next_to_play(to_play_index);
    }


    std::string to_play_name = m_players[to_play_index].m_name;
    std::string last_raise = to_play_name; //first name, in case of all checks 
    
    do {

        publish_to_all(create_move_request(to_play_name));
        listen_and_pull_from_queue();
        //process and send out message;
        if (false){
            last_raise = to_play_name; // will no always be the case
        }
        to_play_index = get_next_to_play(to_play_index);
        to_play_name =  m_players[to_play_index].m_name;
    } while (last_raise != to_play_name);
}


void Smithers::print_players(){
    std::ostringstream message;
    message << '[';
    for (players_cit_t it = m_players.cbegin();
        it !=  m_players.cend();
        it++)
    {

        message << '{'
         << "\"name\":\"" << it->m_name <<"\", "
         << "\"seat\":\"" << it->m_seat <<"\", "
         << "\"chips\":\"" << it->m_chips <<"\" "
         << "},";

    }
    message << "]" << std::endl;
    publish_to_all(message.str());
}
} // smithers namespace