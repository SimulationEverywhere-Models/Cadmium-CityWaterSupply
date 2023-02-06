//Cadmium Simulator headers
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/common_loggers.hpp>

//Time class header
#include <NDTime.hpp>

//Atomic model headers
#include <cadmium/basic_model/pdevs/iestream.hpp> //Atomic model for inputs
#include "../atomics/reservoir.hpp"
#include "../atomics/city_pump.hpp"

//C++ libraries
#include <iostream>
#include <string>

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;

/***** Define input port for coupled models *****/

/***** Define output ports for coupled model *****/
struct top_out_flow: public out_port<float>{}; // Flow
struct top_out_lvl:  public out_port<float>{}; // Level

/****** Input Reader atomic model declaration *******************/
template<typename T>
class InputReader_start : public iestream_input<int,T> {
    public:
        InputReader_start () = default;
        InputReader_start (const char* file_path) : iestream_input<int,T>(file_path) {}
};

int main(){

    /****** Input Reader atomic model instantiation *******************/
    const char * city_pump_input_data  = "../input_data/city_pump_instr.txt";
    shared_ptr<dynamic::modeling::model> city_pump_input_reader;
    city_pump_input_reader = dynamic::translate::make_dynamic_atomic_model<InputReader_start, TIME, const char*>("city_pump_input_reader", move(city_pump_input_data));

    /****** Atomic model instantiation *******************/
    shared_ptr<dynamic::modeling::model> reservoir1;
    reservoir1 = dynamic::translate::make_dynamic_atomic_model<Reservoir, TIME>("reservoir1");

    shared_ptr<dynamic::modeling::model> city_pump1;
    city_pump1 = dynamic::translate::make_dynamic_atomic_model<CityPump, TIME>("city_pump1");

    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP;
    iports_TOP = {};
    dynamic::modeling::Ports oports_TOP;
    oports_TOP = {typeid(top_out_flow), typeid(top_out_lvl)};
    dynamic::modeling::Models submodels_TOP;
    submodels_TOP = {city_pump_input_reader, reservoir1, city_pump1};
    dynamic::modeling::EICs eics_TOP;
    eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP;
    eocs_TOP = {
        dynamic::translate::make_EOC<Reservoir_defs::level,top_out_lvl>("reservoir1"),
        dynamic::translate::make_EOC<CityPump_defs::flow,top_out_flow>("city_pump1")
    };
    dynamic::modeling::ICs ics_TOP;
    ics_TOP = {
        dynamic::translate::make_IC<iestream_input_defs<int>::out,CityPump_defs::start>("city_pump_input_reader","city_pump1"),
        dynamic::translate::make_IC<Reservoir_defs::level,CityPump_defs::level>("reservoir1","city_pump1"),
        dynamic::translate::make_IC<CityPump_defs::flow,Reservoir_defs::flow_out>("city_pump1","reservoir1")
    };
    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP 
    );

    /*************** Loggers *******************/
    static ofstream out_messages("../simulation_results/city_pump_test_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){          
            return out_messages;
        }
    };
    static ofstream out_state("../simulation_results/city_pump_test_output_state.txt");
    struct oss_sink_state{
        static ostream& sink(){          
            return out_state;
        }
    };
    
    using state=logger::logger<logger::logger_state, dynamic::logger::formatter<TIME>, oss_sink_state>;
    using log_messages=logger::logger<logger::logger_messages, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_mes=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_sta=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_state>;

    using logger_top=logger::multilogger<state, log_messages, global_time_mes, global_time_sta>;

    /************** Runner call ************************/ 
    dynamic::engine::runner<NDTime, logger_top> r(TOP, {0});
    r.run_until(NDTime("06:00:00:000"));
    return 0;
}