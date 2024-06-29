CXX=g++
CXXFLAGS=-I.

OBJECTS=client.o requests.o helpers.o buffer.o

client: $(OBJECTS)
	$(CXX) -o client $(OBJECTS) -Wall

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: client
	./client

clean:
	rm -f *.o client
