CXX = g++
OUTPUT_DIR = out

STACK_SIZE = 8000000

OUTPUT_MAIN = main.exe
OUTPUT_TEST = test.exe
OUTPUT_GUI  = gui.exe
OUTPUT_OPT  = opt.exe

CXXFLAGS = -m64 -mbmi -std=c++20 -O1 -march=native -w -Wno-narrowing
LDFLAGS  = -Wl,--stack,$(STACK_SIZE)

ifeq ($(filter openmp,$(MAKECMDGOALS)),openmp)
    CXXFLAGS += -fopenmp
endif

.PHONY: all main test gui opt run run_test run_gui run_opt clean

all: main

main: compile_main link_main clean run 

test: compile_test link_test clean run_test 

gui: compile_gui link_gui clean run_gui 

opt: compile_opt link_opt clean run_opt

compile_main:
	$(CXX) -c -g main.cpp $(CXXFLAGS) -o main.o

link_main:
	$(CXX) main.o -o $(OUTPUT_DIR)/$(OUTPUT_MAIN) $(CXXFLAGS) $(LDFLAGS)

run:
	.\$(OUTPUT_DIR)\$(OUTPUT_MAIN)

compile_test:
	$(CXX) -c test.cpp $(CXXFLAGS) -o test.o

link_test:
	$(CXX) test.o -o $(OUTPUT_DIR)/$(OUTPUT_TEST) $(LDFLAGS) -lgtest_main -lgtest

run_test:
	.\$(OUTPUT_DIR)\$(OUTPUT_TEST)

compile_gui:
	$(CXX) -c -Ilib websocket.cpp $(CXXFLAGS) -o gui.o

link_gui:
	$(CXX) gui.o -o $(OUTPUT_DIR)/$(OUTPUT_GUI) $(LDFLAGS) -lws2_32

run_gui:
	.\$(OUTPUT_DIR)\$(OUTPUT_GUI)

compile_opt:
	$(CXX) -c -g optimize.cpp $(CXXFLAGS) -o opt.o

link_opt:
	$(CXX) opt.o -o $(OUTPUT_DIR)/$(OUTPUT_OPT) $(CXXFLAGS) $(LDFLAGS)

run_opt:
	.\$(OUTPUT_DIR)\$(OUTPUT_OPT)

clean:
	del main.o test.o gui.o opt.o
