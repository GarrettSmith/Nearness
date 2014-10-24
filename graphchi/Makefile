INCFLAGS = -I/usr/local/include/ -I./src/

CPP = g++
CPPFLAGS = -g -O3 $(INCFLAGS)  -fopenmp -Wall -Wno-strict-aliasing -std=c++11
LINKERFLAGS = -lz
DEBUGFLAGS = -ggdb -O0 $(INCFLAGS)  -fopenmp -Wall -Wno-strict-aliasing -std=c++11
HEADERS=$(wildcard *.h**)


debug: myapps/maximal_clique.cpp $(HEADERS)
	@mkdir -p bin/$(@D)
	$(CPP) $(DEBUGFLAGS) -Imyapps/ myapps/maximal_clique.cpp -o bin/maximal_clique $(LINKERFLAGS)

release: myapps/maximal_clique.cpp $(HEADERS)
	@mkdir -p bin/$(@D)
	$(CPP) $(CPPFLAGS) -Imyapps/ myapps/maximal_clique.cpp -o bin/maximal_clique $(LINKERFLAGS)


clean:
	@rm -rf bin/*

myapps/% : myapps/%.cpp $(HEADERS)
	@mkdir -p bin/$(@D)
	$(CPP) $(CPPFLAGS) -Imyapps/ $@.cpp -o bin/$@ $(LINKERFLAGS)





