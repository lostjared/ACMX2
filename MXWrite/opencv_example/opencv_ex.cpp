#include<iostream>
#include<mxwrite.hpp>
#include<opencv2/opencv.hpp>
#include<chrono>
#include<thread>

int main(int argc, char **argv) {
    if(argc != 3) {
        std::cerr << "Usage: " << argv[0] << " camera index mode\n0 mode for normal mode, 1, for timestamp mode\n";
        return 1;
    }
    int index = std::stoi(argv[1]);
    int mode = std::stoi(argv[2]);

    if(mode != 0 &&  mode != 1) {
        std::cerr << "Invalid mode..\n";
        return 1;
    }

    std::cout << "Initializing Camera: " << index << " mode: " << mode << "...\n";
#ifdef _WIN32
    cv::VideoCapture cap(index, cv::CAP_DSHOW);
#else
    cv::VideoCapture cap(index);
#endif
    if(!cap.isOpened()) {
        std::cerr << "Failed to open camera\n";
        return 1;
    }
    double fps = cap.get(cv::CAP_PROP_FPS);
    if(fps <= 0) fps = 30.0; 
    
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    
    std::cout << "Camera settings: " << width << "x" << height << " @ " << fps << "fps\n";

    Writer writer;
    bool status;

    if(mode == 0)
        status = writer.open("output.mp4", width, height, fps, 5000);
    else if(mode == 1)
        status = writer.open_ts("output.mp4", width, height, fps, 5000); 

    if(!status) {
        std::cerr << "Failed to open file for writing\n";
        return 1;
    }
    cv::namedWindow("Webcam", cv::WINDOW_AUTOSIZE);
    cv::Mat frame;
    bool active = true;
    while(active) {
        if(cap.read(frame)) {
            cv::Mat temp;
            cv::cvtColor(frame, temp, cv::COLOR_BGR2RGBA);
            if(mode == 0)
                writer.write(temp.ptr());
            else if(mode == 1)
                writer.write_ts(temp.ptr());

            cv::imshow("Webcam", frame);
        }
        if(cv::waitKey(1) == 27) {
            active = false;
        }
    }
    writer.close();
    cap.release();
    cv::destroyAllWindows();
    return 0;
}