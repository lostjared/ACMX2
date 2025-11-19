# MXWrite - simple static library for writing RGBA video
# FFmpeg-Based Video Writer

This repository provides a C++ class (`Writer`) that uses the [FFmpeg](https://ffmpeg.org/) libraries to write raw RGBA frames to an MP4 (or TS) file in H.264 format. It supports both a straightforward, frame-by-frame workflow (`open()`, `write()`, and `close()`) and a timestamp-based workflow (`open_ts()`, `write_ts()`, and `close()`).  

## Table of Contents

1. [Overview](#overview)
2. [Dependencies](#dependencies)
3. [Building](#building)
4. [Usage](#usage)
   - [Basic Initialization (`open`/`write`)](#basic-initialization-openwrite)
   - [Timestamp-Based Writing (`open_ts`/`write_ts`)](#timestamp-based-writing-open_tswrite_ts)
5. [Key Implementation Details](#key-implementation-details)
6. [License](#license)

---

## Overview

`Writer` is a C++ class that simplifies encoding and writing video frames to a container file. You can:
- Open an output file (MP4 or TS container).
- Write raw RGBA data (passed in as a pointer) to the file, converting it to YUV420P under the hood.
- Optionally use hardware acceleration (CUDA) if available.
- Close and finalize the file properly.

It can handle both:
- **Frame-by-frame mode**: For applications where you generate or capture frames at a known rate.
- **Timestamp-based mode**: For applications that require precise PTS (presentation timestamp) control, typically when your source frames arrive at irregular intervals.

---

## Dependencies

### Required Libraries

1. **FFmpeg** – The code specifically uses the following components:
   - `libavcodec`
   - `libavformat`
   - `libavutil`
   - `libswscale`
2. **Threads** (C++11 and above) – Uses `<thread>` and `<mutex>` from the standard library.
3. A **C++17 (or higher)** compatible compiler.

To install FFmpeg development libraries on your platform:
- **Ubuntu / Debian**:
  ```bash
  sudo apt-get update
  sudo apt-get install ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
  ```
- **Windows**: 
  - Install via [vcpkg](https://github.com/microsoft/vcpkg), [MSYS2](https://www.msys2.org/), or download and build FFmpeg from source.
- **macOS**: 
  ```bash
  brew install ffmpeg
  ```

---
## Building

 **Configure and build**:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   ```
---

## Usage

You can include the `mxwrite.hpp` header in your own C++ files and link against the library. Below is a quick example:

```cpp
#include "mxwrite.hpp"
#include <vector>

int main() {
    Writer writer;
    // 1. Open an output MP4
    bool ok = writer.open("output.mp4", 1280, 720, 30.0f, "24");  // width=1280, height=720, fps=30, bitrate=24 CRF
    if (!ok) {
        return 1;
    }
    // 2. Prepare or capture frames in RGBA format.
    // For demonstration, we'll just create a dummy buffer of size width * height * 4.
    std::vector<uint8_t> dummyRGBA(1280 * 720 * 4, 255); // all white
    // 3. Write frames
    for (int i = 0; i < 60; ++i) { // e.g., 2 seconds of video at 30 FPS
        writer.write(dummyRGBA.data());
    }
    // 4. Close and finalize
    writer.close();
    return 0;
}
```
