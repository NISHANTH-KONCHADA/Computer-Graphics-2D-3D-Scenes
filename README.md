2D & 3D Computer Graphics 🚂🎡
This repository showcases two distinct projects demonstrating mastery of foundational computer graphics concepts and low-level linear algebra, implemented in C++ using the OpenGL Utility Toolkit (GLUT).

1️⃣ Project 1: Dynamic 2D Amusement Park Scene
This project is a dynamic 2D scene built entirely using rasterization algorithms written from scratch. It avoids built-in OpenGL drawing functions to highlight core algorithmic understanding.

✨ 2D Scene Features
Custom Graphics Algorithms: All drawing primitives (lines, circles, polygons) are implemented using fundamental algorithms:

Bresenham's Line Algorithm

Midpoint Circle Algorithm

Scanline Polygon Fill Algorithm

Day/Night Cycle: Toggle between a sunny day scene and a dark night sky featuring a moon, stationary random stars, and realistic fading fireworks (press N or D).

Animations: Includes flags waving using Shear Transformation, a rotating Ferris Wheel, rolling cars, flying birds, and a moving roller coaster.

2️⃣ Project 2: 3D Railway Station Scene with Custom Math
This project demonstrates proficiency in 3D rendering by implementing a scene using a custom, from-scratch 3D math library for all transformations and camera control.

✨ 3D Scene Features
From-Scratch 3D Math Core: Includes custom implementation of key mathematical components:

Vec3 and Matrix4 structures.

Custom Matrix Multiplication (Matrix4 operator*).

Custom Transformation Functions (Translation, Scaling).

Axis-Angle Rotation: Implemented using the Axis-Angle formula for accurate rotations .

Custom Camera Setup using the custom_look_at function.

Realistic Rendering: Features dynamic OpenGL Lighting, materials, OpenGL Fog, and realistic train Reflection on the ground plane.

Dynamic Elements: Includes a moving train with a Smoke Particle System and a rotating station sign.

🛠️ Compilation Guide (Multi-File Structure)
Prerequisites
Both projects require a C++ compiler (GCC/G++) and the OpenGL Utility Toolkit (GLUT) library configured on your system.

Compiling the 3D Scene
If the 3D math is in a separate header (math_library.h), you must compile and link:

Bash

# Example command for Linux/macOS
g++ railway_station_3d.cpp -o railway_3d -lGL -lGLU -lglut -lm
Compiling the 2D Scene
If the 2D scene is in a single file (amusement_park_2d.cpp):

Bash

# Example command for Linux/macOS
g++ amusement_park_2d.cpp -o park_2d -lGL -lGLU -lglut -lm
📜 License
This project is licensed under the MIT License.

Created by: [NISHANTH]