FLAGS = -w -m64 -mbmi -std=c++20 -O2 -march=native
STACK_SIZE = 8000000  # Adjust as needed
OUTPUT_MAIN = main.exe
OUTPUT_TEST = test.exe
OUTPUT_DIR= out

ifeq ($(filter openmp,$(MAKECMDGOALS)),openmp)
    FLAGS += -fopenmp
endif

all: compile_main link_main clean run

testAll: compile_test link_test clean run_test

compile_main:
	g++ -c -g main.cpp $(FLAGS) -o main.o

link_main:
	g++ main.o -o $(OUTPUT_MAIN) -Wl,--stack,$(STACK_SIZE) $(FLAGS)

run:
	.\$(OUTPUT_MAIN)

compile_test:
	g++ -c test.cpp $(FLAGS) -o test.o

link_test:
	g++ test.o -o out/$(OUTPUT_TEST) -lgtest_main -lgtest -Wl,--stack,$(STACK_SIZE) $(FLAGS)

run_test:
	.\$(OUTPUT_TEST)

clean:
	del main.o test.o