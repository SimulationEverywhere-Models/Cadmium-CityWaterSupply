CC=g++
CFLAGS=-std=c++17

INCLUDECADMIUM=-I ../../cadmium/include
INCLUDEDESTIMES=-I ../../DESTimes/include

#CREATE BIN AND BUILD FOLDERS TO SAVE THE COMPILED FILES DURING RUNTIME
bin_folder := $(shell mkdir -p bin)
build_folder := $(shell mkdir -p build)
results_folder := $(shell mkdir -p simulation_results)

#TARGET TO COMPILE SUBNET TEST
main_top.o: top_model/main.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) top_model/main.cpp -o build/main_top.o
main_reservoir_test.o: test/main_reservoir_test.cpp 
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/main_reservoir_test.cpp -o build/main_reservoir_test.o
main_water_supply_pump_test.o: test/main_water_supply_pump_test.cpp 
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/main_water_supply_pump_test.cpp -o build/main_water_supply_pump_test.o
main_city_pump_test.o: test/main_city_pump_test.cpp 
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) test/main_city_pump_test.cpp -o build/main_city_pump_test.o
tests: main_reservoir_test.o main_water_supply_pump_test.o main_city_pump_test.o
	$(CC) -g -o bin/RESERVOIR_TEST build/main_reservoir_test.o
	$(CC) -g -o bin/WATER_SUPPLY_TEST build/main_water_supply_pump_test.o
	$(CC) -g -o bin/CITY_PUMP_TEST build/main_city_pump_test.o

#TARGET TO COMPILE ONLY ABP SIMULATOR
simulator: main_top.o
	$(CC) -g -o bin/CitySupply build/main_top.o

#TARGET TO COMPILE EVERYTHING
all: simulator tests

#CLEAN COMMANDS
clean:
	rm -f bin/* build/*