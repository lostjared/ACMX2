
# ACMX2
Cross Platform Real time Webcam Filters with OpenGL Shaders

![image](https://github.com/user-attachments/assets/7cdf6c57-0938-49ea-906d-594b48149acb)
![image](https://github.com/user-attachments/assets/ee30f7b4-3255-44a2-b9f7-3efadc0650e8)
![image](https://github.com/user-attachments/assets/c5eec97b-4d3b-4da2-bf23-c0689e9bfe7a)


## Basic Information

A C++ application that applies real-time shaders to video from either a camera device or a video file. It can also optionally record the processed video frames to an output file and save snapshots on demand.
The code is based on the libmx2 (MX Engine) available on GitHub here: https://github.com/lostjared/libmx2

## Motives
I wanted a simple and easy way to apply new shaders to camera and video files. To be able to edit them and build libraries of shaders
without having to restort to the command line or buliding the same basic boilerplate code each time.

---

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Technologies Used](#technologies-used)
- [Building and Running](#building-and-running)
- [Command-Line Arguments](#command-line-arguments)
- [Usage Examples](#usage-examples)
- [Keyboard Controls](#keyboard-controls)
- [Credits](#credits)

---

## Overview

**ACMX2** is a GPU-accelerated video processing tool that leverages OpenGL shader programs to apply custom effects to your camera feed or an input video file. It supports:
- Shader switching on-the-fly.
- Time-based animations (via uniform updates).
- Output recording to a file.
- Taking snapshots (saved as PNG).
- Toggling fullscreen.
- Automatic looping of input video.

---

## Features

1. **Camera or Video File Input**  
   Capture and process live camera feed or load a video file.

2. **Shader Effects**  
   Easily load either:
   - A single fragment shader.
   - A library of fragment shaders listed in an `index.txt` file.

3. **Real-time Rendering**  
   Updates and renders frames in real-time with OpenGL and SDL2.

4. **Recording & Snapshots**  
   - Write processed frames to a video file.
   - Save snapshots on demand.

5. **Configurable Resolution & FPS**  
   Change output resolution and frames-per-second, and optionally stretch the input frames to match.

6. **Keyboard Controls**  
   - Switch shaders up/down.
   - Enable/disable time-based animations.
   - Toggle fullscreen mode.
   - Save snapshots at any time.
7. **Graphical User Interface**
    - Graphical User Interface written  using Qt6
    - Easy to use with Code Editor

---

## Technologies Used

- **C++20** for core logic.
- **OpenGL** for GPU-accelerated rendering.
- **SDL2** for creating the window, handling events, and managing the OpenGL context.
- **OpenCV** for camera/video capture (and some basic image manipulation).
- **FFmpeg (through a custom `ffwrite.hpp` wrapper)** for encoding/writing output video files.
- **Argz** library for command-line argument parsing.
- **C++ STL** for concurrency, file system operations, etc.
- **Qt6** for the GUI.

---

## Building and Running

1. **Dependencies** (high-level):
   - SDL2
   - OpenGL & GLAD 
   - OpenCV
   - FFmpeg development libraries
   - C++20 or later compiler
   - Argz library for command-line parsing (if not bundled with the code)
   - libmx2 - MX2 Engine

## Command-Line Arguments

Below is a summary of the available command-line arguments:

| Short | Long                 | Description                                       | Example                       |
|-------|----------------------|---------------------------------------------------|-------------------------------|
| `-p`  | `--path`             | Specify assets path (shaders, icons, etc.).       | `--path ./assets`             |
| `-r`  | `--resolution`       | Output/Processing resolution (Width x Height).    | `--resolution 1280x720`       |
| `-c`  | `--camera-resolution`| Camera capture resolution (Width x Height).       | `--camera-resolution 640x480` |
| `-d`  | `--device`           | Specify camera device index.                      | `--device 1`                  |
| `-i`  | `--input`            | Input video file. If omitted, uses camera input.  | `--input input.mp4`           |
| `-s`  | `--shaders`          | Path to a shader library (uses `index.txt`).      | `--shaders ./filters`         |
| `-f`  | `--fragment`         | Single fragment shader file to load.              | `--fragment ./frag. glsl`     |
| `-h`  | `--shader`     | Shader index within the shader library.                 | `--shader 2`                  |
| `-e`  | `--prefix`           | Snapshot save prefix path.                        | `--prefix ./snapshots`        |
| `-o`  | `--output`           | Output video file name to record.                 | `--output output.mp4`         |
| `-b`  | `--bitrate`          | Bitrate in Kbps for output video.                 | `--bitrate 5000`              |
| `-u`  | `--fps`              | Frames per second for recording.                  | `--fps 30`                    |
| `-a`  | `--repeat`           | Repeat the input video (loop).                    | `--repeat`                    |
| `-n`  | `--fullscreen`       | Start in fullscreen mode.                         | `--fullscreen`                |

### Notes:
- **Resolution Arguments**:
  - *WidthxHeight* (e.g., `1280x720`).
  - For the camera or output resolution, ensure valid dimensions are passed.
- **Input & Camera**:
  - If `--input` is omitted, the program defaults to the camera device index specified by `--device` (default is 0).
- **Shader Selection**:
  - Use either `--shaders` for a library or `--fragment` for a single shader.
  - If both are provided, the last-specified option is used.
- **Defaults**:
  - If no arguments are supplied, a help message is displayed.
---

## Usage Examples

1. **Use Camera with Default Resolution**  
   ```bash
   ./acmx2
   ```
   - Opens camera device **0** at the default resolution, no output file recorded.

2. **Use Camera, Set Capture Resolution, and Record**  
   ```bash
   ./acmx2 -c 640x480 -o camera_output.mp4
   ```
   - Opens camera at 640x480.
   - Records processed output to `camera_output.mp4`.

3. **Use a Single Fragment Shader with Input Video**  
   ```bash
   ./acmx2 -i myvideo.mp4 -f ./shaders/frag_effect.glsl -r 1280x720 -o processed_output.mp4
   ```
   - Loads and applies `frag_effect.glsl`.
   - Stretches frames to 1280x720, then writes final output to `processed_output.mp4`.

4. **Use a Shader Library and Loop the Video**  
   ```bash
   ./acmx2 -i input.mp4 -s ./filters -h 1 -a
   ```
   - Uses `./filters/index.txt` to load a library of fragment shaders.
   - Starts at shader **index 1** in that library.
   - Loops `input.mp4` when it ends.

5. **Fullscreen Mode**  
   ```bash
   ./acmx2 -i input.mp4 -n
   ```
   - Starts in fullscreen, playing and processing `input.mp4`.

---

## Keyboard Controls

During runtime, the following keyboard controls are supported:

- **Up Arrow**: Switch to previous shader in the library.  
- **Down Arrow**: Switch to next shader in the library.
- **Z**: Save a snapshot (PNG) of the current frame.
- **T**: Toggle time-based animation (enables/disables time uniform).
- **I**: Step forward in time (when time-based animation is disabled).
- **O**: Step backward in time (when time-based animation is disabled).
- **F**: Toggle fullscreen.

Press **ESC** or close the window to quit.

## Credits
- Special thanks to all libraries used:
  - **SDL2**, **OpenGL**, **OpenCV**, **FFmpeg**, and standard C++.

Feel free to report issues or contribute via pull requests on GitHub. Thank you for using **ACMX2**!
