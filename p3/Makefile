CXXFLAGS = -std=c++11

INC=-I/Users/kopbob/dev/tmp/libevent_lib/local/include
LIB=-L/Users/kopbob/dev/tmp/libevent_lib/local/lib

all: proxysrv

proxysrv:
	$(CXX) $(CXXFLAGS) $(LIB) $(INC) -o proxysrv ProxyServer.cpp ProxyConfig.cpp -levent

clean:
	rm proxysrv
s
