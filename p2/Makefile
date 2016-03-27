CXXFLAGS = -std=c++11

all: chatsrv client

client:
	$(CXX) $(CXXFLAGS) -o client Client.cpp

chatsrv:
	$(CXX) $(CXXFLAGS) -o chatsrv Server.cpp

clean:
	rm chatsrv
	rm client
