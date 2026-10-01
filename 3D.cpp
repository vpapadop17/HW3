// 3D Floating Shape - wireframe shapes spinning at the origin
//
// Controls (click on the 3D window first!):
//   S             change shape (pyramid -> triangle -> cube -> pyramid ...)
//   G             show / hide grid
//   A             show / hide axes
//   SPACE         pause / resume spinning
//   LEFT / RIGHT  move the view left / right (camera turns its head)
//   ESC           quit

#include <X11/Xlib.h> //this si teh one I use to open a wondow and draw on it 
#include <X11/keysym.h> //has names for keys 
#include <chrono> //for teh 16ms
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

//these two are for the size of the window which we dont want it to change thats why we make them const
const int WIDTH = 800;
const int HEIGHT = 600;


const double FOCAL = 600.0;
const double AXIS_LENGTH = 25;     // how far the x and y axes are from the middle
const double AXIS_LENGTH_Z = 40;   // the z axis is longer, because of teh thing we talked about in class and how it comes at me so i need to make it larger so its shown

const unsigned long AXIS_COLOR = 0x505050;     
const unsigned long GRID_COLOR = 0xC8C8C8;     
const unsigned long BACKGROUND = 0xFFFFFF;     
const unsigned long RED = 0xE02020;
const unsigned long GREEN = 0x20B020;
const unsigned long BLUE = 0x2040E0;
const unsigned long ORANGE = 0xFF8C00;
const unsigned long PURPLE = 0x9030C0;
const unsigned long TEAL = 0x20B0B0;

//a point in 3D that takes three variables
struct Vec3 {       
    double x, y, z;
};

//a line from a to b 
struct Edge {      
	//these are numbered points  
    int a, b;
    //the color of the line 
    unsigned long color;
};

struct Shape {    
	//vector with points
    vector<Vec3> points;
    //vector with lines
    vector<Edge> edges;
};

//these are created or addressed outside the functions so any function can use them becasue every function needs to use them 
Display* display;
Window window;
GC gc;              // the "pen"
Pixmap buffer;      // off-screen canvas

// camera position
double camX = 5, camY = 7, camZ = 70;   
 //turning head left/right, rotation around y
double camYaw = 0;                     

//rotation functions with sin and cos, changes x and z while y stay the same
// in simple math terms rotation around the y axis 
//the double angle is how far we want to turn it 
Vec3 rotateY(Vec3 p, double angle) {
    double newX = p.x * cos(angle) + p.z * sin(angle);
    double newZ = -p.x * sin(angle) + p.z * cos(angle); //minus because i need it to move to the left 
    return {newX, p.y, newZ};
}

//rotation around the x axis 
Vec3 rotateX(Vec3 p, double angle) {
    double newY = p.y * cos(angle) - p.z * sin(angle); //minus because i again need it to move to the left 
    double newZ = p.y * sin(angle) + p.z * cos(angle);
    return {p.x, newY, newZ};
}

//actually making the shapes

Shape makePyramid(double s) {       //s is the size
	//shape shape makes a list (empty) of points an done of edges
    Shape shape;
    double base = -s / 2;   // height of the flat square bottom eg if size is 16 it will be -8. Th etip is at height 16 so pyramid from -8 to 16
    shape.points = {{-s, base, -s}, {s, base, -s}, {s, base, s}, {-s, base, s}, {0, s, 0}};
    //bottom corners and at the end {0, s, 0} we have the tip 
    //the following function connects those points eg on first one 0 and 1 with color red
    shape.edges = {{0, 1, RED}, {1, 2, BLUE}, {2, 3, RED}, {3, 0, TEAL},   // square bottom
                   {0, 4, GREEN}, {1, 4, ORANGE}, {2, 4, PURPLE}, {3, 4, TEAL}};   // up to the tip
    return shape;
}

Shape makeTriangle(double s) {      // 4 corners and 4 triangle sides
    Shape shape;
    shape.points = {{s, s, s}, {s, -s, -s}, {-s, s, -s}, {-s, -s, s}};
    shape.edges = {{0, 1, RED}, {2, 3, RED},
                   {0, 2, GREEN}, {1, 3, GREEN},
                   {0, 3, BLUE}, {1, 2, BLUE}};
    return shape;
}

Shape makeCube(double s) {    
    Shape shape;
    shape.points = {{-s, -s, -s}, {s, -s, -s}, {s, s, -s}, {-s, s, -s},
                    {-s, -s, s},  {s, -s, s},  {s, s, s},  {-s, s, s}};
    //lines going along x are red, along y green, along z blue
    shape.edges = {{0, 1, RED}, {3, 2, RED}, {4, 5, RED}, {7, 6, RED},
                   {1, 2, GREEN}, {0, 3, GREEN}, {5, 6, GREEN}, {4, 7, GREEN},
                   {0, 4, BLUE}, {1, 5, BLUE}, {2, 6, BLUE}, {3, 7, BLUE},
                   {0, 5, ORANGE}, {1, 4, PURPLE}};
    return shape;
}

//open the app to view it 
void setupWindow() {
	//nullptr is just a pointer that says points to nothing 
    display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        cout << "Could not open the X11 display. Is an X server running?" << endl;
        //stops the program if the connection of address is not happening  
        exit(1);
    }
    int screen = DefaultScreen(display);
    //this makes a window of this size with the white background i needed adn returns an ID number
    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                 0, 0, WIDTH, HEIGHT, 0, BACKGROUND, BACKGROUND);
    //name the window
    XStoreName(display, window, "3D Floating Shape");
    XSelectInput(display, window, KeyPressMask);   //I want to be able to press keys
    XMapWindow(display, window);                   //makes the window appear
    gc = XCreateGC(display, window, 0, nullptr);
    XSetLineAttributes(display, gc, 2, LineSolid, CapRound, JoinRound);   //this sets the thickness of teh shape like teh thickness of teh tool we will draw it with 
    buffer = XCreatePixmap(display, window, WIDTH, HEIGHT, DefaultDepth(display, screen));
}

//3D to 2D
// returns false if the point is behind the camera, basically screenX and screenY is how we get the pixel x and y and the answer true or false
bool project(Vec3 p, int &screenX, int &screenY) {
    //make the camera the origin by subtracting the cameras position from the point, ebcase teh formual needs to know where the things are compared to the camera 
    double x = p.x - camX;
    double y = p.y - camY;
    double z = p.z - camZ;

    //camera angle: turn head left/right (rotate around y)
    Vec3 turned = rotateY({x, y, z}, -camYaw);

    //camera looks along -z, so distance in front of us is -z, ie the camera looks along the negative z direction so points in front of it hae a negative z
    double depth = -turned.z;
    //if depth is smaller than one the point is beneath the camera or on top of it 
    if (depth < 1) return false;

    //far away = smaller
    double X = turned.x * FOCAL / depth;
    double Y = turned.y * FOCAL / depth;

    //what we said in class about the screens origin being at the top left and our y going up but the screen's going down
    screenX = (int)(X + WIDTH / 2);
    screenY = (int)(HEIGHT / 2 - Y);
    return true;
}

//drawing functions
void drawLine3D(Vec3 a, Vec3 b, unsigned long color) {
    int x1, y1, x2, y2;
    //runs on points a and b and checks if the point is in front of the camera
    if (!project(a, x1, y1)) return;
    if (!project(b, x2, y2)) return;
    XSetForeground(display, gc, color);
    XDrawLine(display, buffer, gc, x1, y1, x2, y2);
}

void drawShape(Shape shape, double spinY, double spinX) {
    // spin every point around the middle of the shape
    for (int i = 0; i < (int)shape.points.size(); i++) {
        shape.points[i] = rotateY(shape.points[i], spinY);
        shape.points[i] = rotateX(shape.points[i], spinX);
    }
    // draw a line for every edge, in that edge's color
    for (int i = 0; i < (int)shape.edges.size(); i++) {
        Vec3 start = shape.points[shape.edges[i].a];
        Vec3 end = shape.points[shape.edges[i].b];
        drawLine3D(start, end, shape.edges[i].color);
    }
}

void drawGrid() {
    for (int i = -60; i <= 60; i += 10) {
        drawLine3D({(double)i, -20, -60}, {(double)i, -20, 60}, GRID_COLOR);
        drawLine3D({-60, -20, (double)i}, {60, -20, (double)i}, GRID_COLOR);
    }
}

void drawAxes() {
    // long lines through the origin. They never move or spin. Only the camera does.
    drawLine3D({-AXIS_LENGTH, 0, 0}, {AXIS_LENGTH, 0, 0}, AXIS_COLOR);   // x
    drawLine3D({0, -AXIS_LENGTH, 0}, {0, AXIS_LENGTH, 0}, AXIS_COLOR);   // y
    drawLine3D({0, 0, -AXIS_LENGTH_Z}, {0, 0, AXIS_LENGTH_Z}, AXIS_COLOR);   //z (positive and end points towards me)
}

//
int main() {
    setupWindow();

    Shape pyramid = makePyramid(8);
    Shape triangle = makeTriangle(7);
    Shape cube = makeCube(7);
    int current = 1;   // 1 = pyramid, 2 = triangle, 3 = cube

	//how far the shape has turned so far 
    double spinY = 0, spinX = 0;
    bool showGrid = false, showAxes = true, spinning = true, running = true;

    while (running) {
        auto frameStart = chrono::steady_clock::now();

        //
        while (XPending(display)) {
			//Makes a variable to hold an event, then takes the next waiting event from X11 and puts it in that variable. &event hands the variable's address, so the function can fill it.
            XEvent event;
            XNextEvent(display, &event);
            if (event.type == KeyPress) {
                KeySym key = XLookupKeysym(&event.xkey, 0);
                if (key == XK_Escape) running = false;
                if (key == XK_s) {                 // next shape
                    current = current + 1;
                    if (current > 3) current = 1;  //after the cube goes back to the pyramid
                }
                if (key == XK_g) showGrid = !showGrid;
                if (key == XK_a) showAxes = !showAxes;
                if (key == XK_space) spinning = !spinning;
                // move the view left/right with the arrow keys (the shape and axis can leave the screen according to the file Prof gave us)
                if (key == XK_Right) camYaw += 0.04;    //shape and axis go right
                if (key == XK_Left) camYaw -= 0.04;     //shape and axis go left
            }
        }

        if (spinning) {
			//I show in the program Prof uploaded for us that wehn we spinned it tilted a little bit and that is what this does 
            spinY += 0.02;
            spinX += 0.01;
        }

		//Set the pen to white, then fill a rectangle covering the entire buffer, from (0, 0) with size WIDTH by HEIGHT. This erases the previous frame.
        XSetForeground(display, gc, BACKGROUND);
        XFillRectangle(display, buffer, gc, 0, 0, WIDTH, HEIGHT);

        if (showGrid) drawGrid();
        if (showAxes) drawAxes();
        if (current == 1) drawShape(pyramid, spinY, spinX);
        if (current == 2) drawShape(triangle, spinY, spinX);
        if (current == 3) drawShape(cube, spinY, spinX);

        //show the buffer on the window
        XCopyArea(display, buffer, window, gc, 0, 0, WIDTH, HEIGHT, 0, 0);
        XFlush(display);

        //wait so each frame takes 16 ms
        auto elapsed = chrono::steady_clock::now() - frameStart;
        auto target = chrono::milliseconds(16);
        if (elapsed < target) this_thread::sleep_for(target - elapsed);
    }

    XCloseDisplay(display);
    return 0;
}
