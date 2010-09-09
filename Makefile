


all: main.c analyzer.c analyzer.h
	gcc -o analyzer main.c analyzer.c

clean:
	rm -rf analyzer
