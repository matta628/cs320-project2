FLAGS = -Wall -Wextra -pedantic -DDEBUG -std=c++14

all: cache-sim

cache-sim: cache-sim.cpp
	g++ $(FLAGS) cache-sim.cpp -o cache-sim
clean:
	rm -rf cache-sim output.txt
