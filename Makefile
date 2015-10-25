make :
	g++ transmitter.cpp -o transmitter -std=c++11 -pthread
	g++ receiver.cpp -o receiver -std=c++11 -pthread
