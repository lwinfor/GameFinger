INCLUDE =./include/
SRC     =./src/
CXX = g++
TARGET =server client

SEROBJ=server.o function.o
CLIOBJ=client.o function.o

all:$(TARGET)

server:$(SEROBJ)
	$(CXX) -g -o server $(SEROBJ)

server.o :$(SRC)server.cpp
	$(CXX) -c $(SRC)server.cpp -I$(INCLUDE)

function.o:$(SRC)function.cpp $(INCLUDE)function.h $(INCLUDE)utility.h
	$(CXX) -c $(SRC)function.cpp -I$(INCLUDE)


client:$(CLIOBJ)
	$(CXX) -g -o client $(CLIOBJ)

client.o:$(SRC)client.cpp
	$(CXX) -c $(SRC)client.cpp -I$(INCLUDE)

.PHONY :clean
clean :
	-rm $(TARGET) *.o