/**
 * James Baak
 * SYSC5104 - Carleton University
 * 
 * Cadmium implementation of CD++ atomic models from City Water Supply (modified by author)
 * See documentation for further details of modifications
**/

#ifndef  _WATER_SUPPLY_PUMP_HPP__
#define _WATER_SUPPLY_PUMP_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <random>

using namespace cadmium;
using namespace std;

// Port Definition
struct WaterSupplyPump_defs {
    struct start : public in_port<int> {};
    struct level : public in_port<float> {};
    struct flow  : public out_port<float> {};
};

template<typename TIME> class WaterSupplyPump {
    public:
    // Ports
    using input_ports  = tuple<typename WaterSupplyPump_defs::start, typename WaterSupplyPump_defs::level>;
    using output_ports = tuple<typename WaterSupplyPump_defs::flow>;
    // State
    struct state_type {
        bool  active;
        float flow;
        float period;
        bool  blockage;
        float max_level;
        bool wait;
    };
    state_type state;
    // Constructor
    WaterSupplyPump() {
        state.active = false;
        state.flow = 0.5; //m^3 / s
        state.period = 30.0; // seconds
        state.blockage = false;
        state.max_level = 5.0;
        state.wait = false;
    }
    // internal transition
    void internal_transition() { 
        if (state.blockage) {
            state.blockage = false;
        }
    }
    // external transition
    void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        vector<int> start = get_messages<typename WaterSupplyPump_defs::start>(mbs);
        vector<float> level = get_messages<typename WaterSupplyPump_defs::level>(mbs);
        if(start.size()>1 || level.size()>1) assert(false && "One message at a time");               
        
        if (start.size() > 0) {
            if (start[0] == 1) {
                state.active = true;
            } else {
                state.active = false;
            }
        }
        if (level.size() > 0) {
            if (level[0] >= (state.max_level - 0.2)) { // Stop just before the reservoir is full
                state.wait = true;
            } else if (level[0] <= (state.max_level - 0.5)) { // Restart pump after level drops a bit
                state.wait = false;
            }
        }
        // Chance to generate blockage in water supply pipes
        if ((double)rand() / (double) RAND_MAX  < 0.9){                
            state.blockage = false;
        }else{
            state.blockage = true;
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
        get_messages<typename WaterSupplyPump_defs::flow>(bags) = flow;
        return bags;
    }
    // time_advance function
    TIME time_advance() const {
        TIME next_internal;
        if (state.active && !state.wait) {
            if (state.blockage) {            
                next_internal = TIME("00:30:00:000"); // Time it takes to unblock
            } else {
                next_internal = TIME("00:00:30:000"); // Time until next packet of water
            }    
        } else {
            next_internal = numeric_limits<TIME>::infinity();
        }
        return next_internal;
    }

    friend ostringstream& operator<<(ostringstream& os, const typename WaterSupplyPump<TIME>::state_type& i) {
        os << "active: " << i.active << " & blockage: " << i.blockage << " & waiting: " << i.wait; 
        return os;
    }
};
#endif // _WATER_SUPPLY_PUMP__
