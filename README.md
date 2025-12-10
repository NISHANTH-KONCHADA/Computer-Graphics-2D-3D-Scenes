# 🎨🚂 2D & 3D Computer Graphics Projects (OpenGL + GLUT)

This repository contains two graphics projects demonstrating **from-scratch algorithms**, **custom 3D math**, and **interactive animations**, all implemented in **C++ using OpenGL/GLUT**.

---

# 🎡 Project 1 — 2D Amusement Park  
**File:** `2d_scene_amusement_park.cpp`

A fully animated 2D scene drawn using **manual rasterization algorithms** (no OpenGL primitives).

### 🔧 Custom Algorithms
- Bresenham Line Algorithm  
- Midpoint Circle Algorithm     
- Scanline Polygon Fill  

### ✨ Features
- **Day/Night mode** (`N` / `D`)  
- **Ferris wheel**, **cars**, **birds**, **roller coaster**  
- **Waving flags** using shear transformation  
- **Fireworks** (night mode)  
- Moving sun, clouds, and multiple environment elements  

---

# 🚉 Project 2 — 3D Railway Station  
**File:** `3d_scene_CinematicStation.cpp`

A cinematic 3D scene built using a **custom 3D math library** instead of OpenGL’s transformation functions.

### 🔧 Custom 3D Math
- `Vec3` operations  
- `Matrix4` (translation, scaling, axis-axis rotation)  
- Manual matrix stack  
- Custom `lookAt` camera  

### ✨ Features
- Animated **train** with multiple coaches  
- **Smoke particle system**  
- **Rotating station sign**  
- Trees, platform, tracks, and passengers  
- OpenGL **lighting**, **materials**, and **fog**  
- **Reflection** of the train on the ground  

---

# 🧾 Compilation

### 2D Scene
```bash
g++ 2d_scene_amusement_park.cpp -o park_2d -lGL -lGLU -lglut -lm
```
### 3D Scene
```bash
g++ 3d_scene_CinematicStation.cpp -o railway_3d -lGL -lGLU -lglut -lm
<<<<<<< Updated upstream
```
=======
```
>>>>>>> Stashed changes
