#ifndef SMITHERS
#define SMITHERS

#include "player.h"
#include "messages.h"

#include "hand_rank.h"

#include <zmq.hpp>
#include <json/json.h>

#include <string>
#include <vector>



namespace smithers{
    typedef std::vector<Player>::iterator players_it_t;
    typedef std::vector<Player>::const_iterator players_cit_t;

    
class Smithers{
    
    public:
        Smithers();

        void await_registered_players(int max_players, int max_chips );
        void play_tournament(int chips, int min_raise, int hands_before_blind_double);
        
        void publish_to_all(const std::string& message);
        void publish_to_all(const Json::Value& json);

        void print_players();

    private:

        Json::Value listen_and_pull_from_queue(const std::string& player_name);

        std::vector<Player> m_players;
        zmq::context_t m_zmq_context;
        zmq::socket_t m_publisher;



    };




}

#endif 