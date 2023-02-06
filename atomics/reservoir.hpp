/**
 * James Baak
 * SYSC5104 - Carleton University
 * 
 * Cadmium implementation of CD++ atomic models from City Water Supply (modified by author)
 * See documentation for further details of modifications
**/

#ifndef  _RESERVOIR_HPP__
#define _RESERVOIR_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <random>

using namespace cadmium;
using namespace std;

// Port Definition
struct Reservoir_defs {
    struct flow_out : public in_port<float> {};
    struct flow_in : public in_port<float> {};
    struct level : public out_port<float> {};
};

template<typename TIME> class Reservoir {
    public:
    // Ports
    using input_ports  = tuple<typename Reservoir_defs::flow_in, typename Reservoir_defs::flow_out>;
    using output_ports = tuple<typename Reservoir_defs::level>;
    // State
    struct state_type {
        float volume; // In m^3
        bool reading;
        float surface;
        float height;
    };
    state_type state;
    // Constructor
    Reservoir() {
        state.volume = 1000;
        state.reading = false;
        state.surface = 10.0 * 100.0;
        state.height = 5.0;
    }
    // internal transition
    void internal_transition() { 
        state.reading = false;
    }
    // external transition
    void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) { 
        vector<float> flow_in;
        vector<float> flow_out;
        flow_in  = get_messages<typename Reservoir_defs::flow_in>(mbs);
        flow_out = get_messages<typename Reservoir_defs::flow_out>(mbs);
        // if(flow_in.size()>1 || flow_out.size()>1) assert(false && "One message at a time");               
        state.reading = true;
        // Can handle multiple arrive and departure of water packets
        if (flow_in.size() > 0) {
            for (int i = 0; i < flow_in.size(); i++) {
                state.volume += flow_in[i];
            }
        }
        if (flow_out.size() > 0) {
            for (int i = 0; i < flow_out.size(); i++) {
                state.volume -= flow_out[i];
            }
        }
        assert(state.volume <= state.height * state.surface); // Ensure reservoir is not overflowing
    }
    // confluence transition
    void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        internal_transition();
        external_transition(TIME(), move(mbs));
    }
    // output function
    typename make_message_bags<output_ports>::type output() const {
        typename make_message_bags<output_ports>::type bags;
        vector<float> level;            
        level.push_back(state.volume / state.surface);
        get_messages<typename Reservoir_defs::level>(bags) = level;
        return bags;
    }
    // time_advance function
    TIME time_advance() const {
        TIME next_internal;
        if (state.reading) {            
            next_internal = TIME("00:00:03:000"); // Water level sensor reading time
        } else {
            next_internal = numeric_limits<TIME>::infinity();
        }    
        return next_internal;
    }

    friend ostringstream& operator<<(ostringstream& os, const typename Reservoir<TIME>::state_type& i) {
        os << "volume: " << i.volume << " & level: " << i.volume / i.surface; 
        return os;
    }
};
#endif // _RESERVOIR_HPP__
