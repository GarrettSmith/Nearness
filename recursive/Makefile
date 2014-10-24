INCFLAGS = -I./src/ -I./lib/
INC_DIR = -I/usr/local/include/
LIBS = -L/usr/lib/ -lboost_program_options -lboost_system -lboost_filesystem -lboost_thread -lpthread -L./lib/libhungarian_c -lhungarian

WIN64_CPP = x86_64-w64-mingw32-g++
WIN64_BOOST_DIR = /home/garrett/dev/boost_1_49_0/win64
WIN64_LIBS = -L$(WIN64_BOOST_DIR)/lib/ -lboost_program_options-mt -lboost_system-mt -lboost_filesystem-mt -lboost_thread_win32-mt

WIN32_CPP = i686-w64-mingw32-g++
WIN32_BOOST_DIR = /home/garrett/dev/boost_1_49_0/win32
WIN32_LIBS = -L$(WIN32_BOOST_DIR)/lib/ -lboost_program_options-mt -lboost_system-mt -lboost_filesystem-mt -lboost_thread_win32-mt

CPP = g++

CPPFLAGS = -g -O3 $(INCFLAGS) -Wall -Wno-strict-aliasing
LINKERFLAGS = -lz
DEBUGFLAGS = -ggdb -O0 $(INCFLAGS)  -Wall -Wno-strict-aliasing
HEADERS=$(wildcard *.h**)

release: src/nearness.cpp $(HEADERS)
	@mkdir -p bin/$(@D)
	$(CPP) $(CPPFLAGS) $(INC_DIR) src/nearness.cpp -o bin/nearness $(LINKERFLAGS) $(LIBS)

debug: src/nearness.cpp $(HEADERS)
	@mkdir -p bin/$(@D)
	$(CPP) $(DEBUGFLAGS) $(INC_DIR) src/nearness.cpp -o bin/nearness $(LINKERFLAGS) $(LIBS)

win64: src/nearness.cpp $(HEADERS)
	@mkdir -p bin/win64/$(@D)
	$(WIN64_CPP) $(CPPFLAGS) -I$(WIN64_BOOST_DIR)/include/ src/nearness.cpp -o bin/win64/nearness_win64.exe $(WIN64_LINKERFLAGS) $(WIN64_LIBS)

win32: src/nearness.cpp $(HEADERS)
	@mkdir -p bin/win32/$(@D)
	$(WIN32_CPP) $(CPPFLAGS) -I$(WIN32_BOOST_DIR)/include/ src/nearness.cpp -o bin/win32/nearness_win32.exe $(WIN32_LINKERFLAGS) $(WIN32_LIBS)

clean:
	@rm -rf bin/*

% : src/%.cpp $(HEADERS)
	@mkdir -p bin/$(@D)
	$(CPP) $(CPPFLAGS) $(INC_DIR) src/$@.cpp -o bin/$@ $(LINKERFLAGS) $(LIB_DIR) $(LIBS)

win64_maximal_clique: src/maximal_clique.cpp $(HEADERS)
	@mkdir -p bin/win64/$(@D)
	$(WIN64_CPP) $(CPPFLAGS) -I$(WIN64_BOOST_DIR)/include/ src/maximal_clique.cpp -o bin/win64/maximal_clique_win64.exe $(WIN64_LINKERFLAGS) $(WIN64_LIBS)

win32_maximal_clique: src/maximal_clique.cpp $(HEADERS)
	@mkdir -p bin/win32/$(@D)
	$(WIN32_CPP) $(CPPFLAGS) -I$(WIN32_BOOST_DIR)/include/ src/maximal_clique.cpp -o bin/win32/maximal_clique_win32.exe $(WIN32_LINKERFLAGS) $(WIN32_LIBS)
