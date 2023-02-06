/**
 * James Baak
 * SYSC5104 - Carleton University
 * 
 * Cadmium implementation of CD++ atomic models from City Water Supply (modified by author)
 * See documentation for further details of modifications
**/

#ifndef  _CITY_PUMP_HPP__
#define  _CITY_PUMP_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <random>

using namespace cadmium;
using namespace std;

// Port Definition
struct CityPump_defs {
    struct start : public in_port<int> {}; // Start can be used to simulate start, stop, and power commands
    struct level : public in_port<float> {};
    struct flow  : public out_port<float> {};
};

template<typename TIME> class CityPump {
    public:
    // Ports
    using input_ports  = tuple<typename CityPump_defs::start, typename CityPump_defs::level>;
    using output_ports = tuple<typename CityPump_defs::flow>;
    // State
    struct state_type {
        bool  active;
        float flow;
        float period;
        float min_level;
        bool wait;
    };
    state_type state;
    // Constructor
    CityPump() {
        state.active = false;
        state.flow = 0.4; //m^3 / s
        state.period = 30.0; // seconds
        state.min_level = 0.5;
        state.wait = false;
    }
    // internal transition
    void internal_transition() {
    }
    // external transition
    void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        vector<int> start = get_messages<typename CityPump_defs::start>(mbs);
        vector<float> level = get_messages<typename CityPump_defs::level>(mbs);
        if(start.size()>1 || level.size()>1) assert(false && "One message at a time");               
        
        if (start.size() > 0) {
            if (start[0] == 1) {
                state.active = true;
            } else {
                state.active = false;
            }
        }
        if (level.size() > 0) {
            if (level[0] <= state.min_level) { // Stop just before the reservoir is full
                state.wait = true;
            } else if (level[0] >= (state.min_level + 0.2)) { // Restart pump after level increases a bit
                state.wait = false;
            }
        }
    }
    // confluence transition
    void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        internal_transition();
        external_transition(TIME(), move(mbs));
    }
    // output function
    typename make_message_bags<output_ports>::type output() const {
        typename make_message_bags<output_ports>::type bags;
        vector<float> flow;            
        flow.push_back(state.flow * state.period);
        get_messages<typename CityPump_defs::flow>(bags) = flow;
        return bags;
    }
    // time_advance function
    TIME time_advance() const {
        TIME next_internal;
        if (state.active && !state.wait) {
            next_internal = TIME("00:00:30:000"); // Time until next packet of water
        } else {
            next_internal = numeric_limits<TIME>::infinity();
        }
        return next_internal;
    }

    friend ostringstream& operator<<(ostringstream& os, const typename CityPump<TIME>::state_type& i) {
        os << "active: " << i.active; 
        return os;
    }
};
#endif // _CITY_PUMP__
