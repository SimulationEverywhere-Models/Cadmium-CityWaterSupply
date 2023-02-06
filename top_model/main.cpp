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
#include "../atomics/water_supply_pump.hpp"
#include "../atomics/city_pump.hpp"

//C++ headers
#include <iostream>
#include <chrono>
#include <algorithm>
#include <string>


using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;

/***** Define input port for coupled models *****/
struct start_city_pumps : public in_port<int>{};
struct start_supply_pumps : public in_port<int>{};
struct supply_flow_in : public in_port<float>{};
struct supply_level : public in_port<float>{};
/***** Define output ports for coupled models *****/
struct level : public out_port<float>{};
struct flow_in : public out_port<float>{};

/****** Input Reader atomic model declaration *******************/
template<typename T>
class InputReader_Int : public iestream_input<int,T> {
public:
    InputReader_Int() = default;
    InputReader_Int(const char* file_path) : iestream_input<int,T>(file_path) {}
};

int main(int argc, char ** argv) {

    if (argc < 2) {
        cout << "Program used with wrong parameters. The program must be invoked as follow:";
        cout << argv[0] << " path to the input file " << endl;
        return 1; 
    }
    /****** Input Readers atomic model instantiation *******************/
    string input_1 = argv[1];
    const char * i_input_1 = input_1.c_str();
    shared_ptr<dynamic::modeling::model> pumps_input_reader  = dynamic::translate::make_dynamic_atomic_model<InputReader_Int, TIME, const char* >("pumps_input_reader" , move(i_input_1));

    string input_2 = argv[2];
    const char * i_input_2 = input_2.c_str();
    shared_ptr<dynamic::modeling::model> supply_input_reader = dynamic::translate::make_dynamic_atomic_model<InputReader_Int, TIME, const char* >("supply_input_reader" , move(i_input_2));

    /****** Reservoir atomic model instantiation *******************/
    shared_ptr<dynamic::modeling::model> reservoir1 = dynamic::translate::make_dynamic_atomic_model<Reservoir, TIME>("reservoir1");

    /****** Water Supply Pumps atomic model instantiation *******************/
    shared_ptr<dynamic::modeling::model> supply1 = dynamic::translate::make_dynamic_atomic_model<WaterSupplyPump, TIME>("supply1");
    shared_ptr<dynamic::modeling::model> supply2 = dynamic::translate::make_dynamic_atomic_model<WaterSupplyPump, TIME>("supply2");

    /****** City Pumps atomic models instantiation *******************/
    shared_ptr<dynamic::modeling::model> pump1 = dynamic::translate::make_dynamic_atomic_model<CityPump, TIME>("pump1");
    shared_ptr<dynamic::modeling::model> pump2 = dynamic::translate::make_dynamic_atomic_model<CityPump, TIME>("pump2");

    /*******Water Supply COUPLED MODEL********/
    dynamic::modeling::Ports iports_Supply = {typeid(start_supply_pumps),typeid(supply_level)};
    dynamic::modeling::Ports oports_Supply = {typeid(flow_in)};
    dynamic::modeling::Models submodels_Supply = {supply1, supply2};
    dynamic::modeling::EICs eics_Supply = {
        dynamic::translate::make_EIC<start_supply_pumps, WaterSupplyPump_defs::start>("supply1"),
        dynamic::translate::make_EIC<start_supply_pumps, WaterSupplyPump_defs::start>("supply2"),
        dynamic::translate::make_EIC<supply_level, WaterSupplyPump_defs::level>("supply1"),
        dynamic::translate::make_EIC<supply_level, WaterSupplyPump_defs::level>("supply2"),
    };
    dynamic::modeling::EOCs eocs_Supply = {
        dynamic::translate::make_EOC<WaterSupplyPump_defs::flow,flow_in>("supply1"),
        dynamic::translate::make_EOC<WaterSupplyPump_defs::flow,flow_in>("supply2")
    };
    dynamic::modeling::ICs ics_Supply = {};
    shared_ptr<dynamic::modeling::coupled<TIME>> WaterSupply;
    WaterSupply = make_shared<dynamic::modeling::coupled<TIME>>(
        "WaterSupply", submodels_Supply, iports_Supply, oports_Supply, eics_Supply, eocs_Supply, ics_Supply 
    );

    /*******Pump Station COUPLED MODEL********/
    dynamic::modeling::Ports iports_PumpStation = {typeid(start_city_pumps),typeid(supply_flow_in)};
    dynamic::modeling::Ports oports_PumpStation = {typeid(level)};
    dynamic::modeling::Models submodels_PumpStation = {pump1, pump2, reservoir1};
    dynamic::modeling::EICs eics_PumpStation = {
        cadmium::dynamic::translate::make_EIC<start_city_pumps, CityPump_defs::start>("pump1"),
        cadmium::dynamic::translate::make_EIC<start_city_pumps, CityPump_defs::start>("pump2"),
        cadmium::dynamic::translate::make_EIC<supply_flow_in, Reservoir_defs::flow_in>("reservoir1"),
    };
    dynamic::modeling::EOCs eocs_PumpStation = {
        dynamic::translate::make_EOC<Reservoir_defs::level,level>("reservoir1")
    };
    dynamic::modeling::ICs ics_PumpStation = {
        dynamic::translate::make_IC<CityPump_defs::flow, Reservoir_defs::flow_out>("pump1","reservoir1"),
        dynamic::translate::make_IC<CityPump_defs::flow, Reservoir_defs::flow_out>("pump2","reservoir1"),
        dynamic::translate::make_IC<Reservoir_defs::level, CityPump_defs::level>("reservoir1", "pump1"),
        dynamic::translate::make_IC<Reservoir_defs::level, CityPump_defs::level>("reservoir1", "pump2"),
    };
    shared_ptr<dynamic::modeling::coupled<TIME>> PumpStation;
    PumpStation = make_shared<dynamic::modeling::coupled<TIME>>(
        "PumpStation", submodels_PumpStation, iports_PumpStation, oports_PumpStation, eics_PumpStation, eocs_PumpStation, ics_PumpStation 
    );


    /*******TOP COUPLED MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {typeid(level)};
    dynamic::modeling::Models submodels_TOP = {WaterSupply, PumpStation, supply_input_reader, pumps_input_reader};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP = {
        dynamic::translate::make_EOC<level,level>("PumpStation")
    };
    dynamic::modeling::ICs ics_TOP = {
        dynamic::translate::make_IC<flow_in, supply_flow_in>("WaterSupply","PumpStation"),
        dynamic::translate::make_IC<level, supply_level>("PumpStation", "WaterSupply"),
        dynamic::translate::make_IC<iestream_input_defs<int>::out, start_supply_pumps>("supply_input_reader","WaterSupply"),
        dynamic::translate::make_IC<iestream_input_defs<int>::out, start_city_pumps>("pumps_input_reader","PumpStation"),
    };
    shared_ptr<cadmium::dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP 
    );

    /*************** Loggers *******************/
    static ofstream out_messages("../simulation_results/City_Supply_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){          
            return out_messages;
        }
    };
    static ofstream out_state("../simulation_results/City_Supply_output_state.txt");
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
    r.run_until(NDTime("24:00:00:000"));
    return 0;
}