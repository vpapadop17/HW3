# HW3: 3D Floating Shape

A C++ program that draws a spinning 3D wireframe shape using Xlib (X11).
It has three shapes: a pyramid, a triangle (triangle-based pyramid), and a cube.

## How to compile and run

make try

Or by hand:

g++ -std=c++17 -Wall 3D.cpp -o 3d -I/opt/X11/include -L/opt/X11/lib -lX11
./3d

## Controls

Click the 3D window first, then:

- S: change shape
- G: show / hide grid
- A: show / hide axes
- Space: pause / resume spinning
- Left / Right arrows: turn the camera
- Esc: quit

## How it works

- Each shape is a list of 3D points and a list of lines between them.
- Points are rotated with sin and cos, then projected to the screen
  (x * focal length / distance) so far things look smaller.
- Each frame is drawn on an off-screen buffer, then copied to the window.
- Each frame takes 16 ms.
