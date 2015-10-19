all:
	make receiver
	make transmitter
receiver:
	rm -f receiver
	g++ receiver.cpp -o receiver -pthread
transmitter:
	g++ transmitter.cpp -o transmitter -pthread
receiverTest:
	./receiver 5555
transmitterTest:
	./transmitter 127.0.0.1 5555 coba.txt
