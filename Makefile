all:
	g++ -std=c++17 -Wall 3D.cpp -o 3d -I/opt/X11/include -L/opt/X11/lib -lX11

run:
	./3d

try: all run
