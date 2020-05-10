all: fps

fps: fps.cpp timers.cpp
	g++ fps.cpp libggfonts.a timers.cpp -Wall -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lGL -lGLU -lm -ofps

clean:
	rm -f fps
	rm -f *.o

