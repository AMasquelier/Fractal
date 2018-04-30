all: exec

exec: main.c lib
	gcc -o main main.c libfractal/libfractal.a -Ilibfractal/ -lSDL -lpthread

lib:
	cd libfractal && $(MAKE)

clean:
	rm Fractal
	cd libfractal
