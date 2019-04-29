all: auto_grader

auto_grader:test.c thread_lib 
	gcc -g test.c threads.o -o auto_grader

thread_lib:pthread.c
	gcc -c pthread.c -o threads.o

clean:
	rm auto_grader threads.o