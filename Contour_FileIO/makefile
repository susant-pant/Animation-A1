OS_NAME:=$(shell uname -s)

ifeq ($(OS_NAME),Linux)
	LIBS +=
endif

target a.out:
	g++ -std=c++11 main.cpp -Wall -Wpragmas $(LIBS) -o a.out

clean:
	rm *.o
