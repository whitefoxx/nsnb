nsnb: server client
client:
	gcc -Wall src/client.c -o bin/client
server: server.o
	g++ -Wall tmp/server.o -o bin/server -L/usr/lib -L/opt/local/lib -ltokyocabinet -lz -lbz2 -lpthread -lm -lc
server.o:
	g++ -Wall -I/urs/include -I/opt/local/include -c src/server.cc -o tmp/server.o
clean:
	-rm tmp/*
	-rm db/*
	-rm bin/*
