all: auto_grader

auto_grader:autograder_main.c thread_lib 
	gcc -g autograder_main.c threads.o -o auto_grader

thread_lib:pthread.c
	gcc -g -c pthread.c -o threads.o

clean:
	rm auto_grader threads.o