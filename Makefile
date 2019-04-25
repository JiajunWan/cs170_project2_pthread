all: auto_grader

auto_grader:autograder_main.c thread_lib 
	g++ autograder_main.c threads.o -o auto_grader

thread_lib:threads.cpp
	g++ -c threads.cpp -o threads.o
