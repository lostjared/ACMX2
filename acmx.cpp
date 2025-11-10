#include "version_info.hpp"
#include <mx.hpp>
#include <argz.hpp>
#include <gl.hpp>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <chrono>
#include <thread>
#include <ctime>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <deque>
#include <mxwrite.hpp>
#ifdef AUDIO_ENABLED
#include "audio.hpp"
#endif
#include<string_view>
#include <deque>
#include <opencv2/opencv.hpp>
#include <model.hpp>

void transfer_audio(std::string_view, std::string_view);

class FrameCache {
public:
    explicit FrameCache(std::size_t num)
        : num_frames(num)  {
    }
    ~FrameCache() = default;
    void push(cv::Mat&& frame) {
        if (frames.size() < num_frames) {
            frames.emplace_back(std::move(frame));
        } else {
            frames.pop_front();
            frames.emplace_back(std::move(frame));
        }
    }
    cv::Mat& at(std::size_t index) {
        return frames.at(index);
    }
    cv::Mat& operator[](std::size_t index) { return frames[index]; }
    std::size_t size() const {
        return frames.size();
    }

    bool isFull() {
        if(size() == num_frames) 
            return true;
        return false;
    }

    void fill(cv::Mat &frame) {
        for(size_t i = 0; i < num_frames; ++i) {
            if(frames.size() < num_frames)
                frames.push_back(frame);
        }
    }

private:
    std::size_t num_frames;       
    std::deque<cv::Mat> frames;   
};

class ShaderLibrary {
    float alpha = 1.0;
    float time_f = 1.0;
    bool time_active = true;
    bool is3d = false;
public:
    ShaderLibrary() = default;
    ~ShaderLibrary() {}

    void loadProgram(gl::GLWindow *win, const std::string text) {
        programs.push_back(std::make_unique<gl::ShaderProgram>());
        if(is3d == true) {
            if(!programs.back()->loadProgram(win->util.getFilePath("data/vertex.glsl"), text)) {
                throw mx::Exception("Error loading shader program: " + text);
            }
        } else {
            if(!programs.back()->loadProgram(win->util.getFilePath("data/vert.glsl"), text)) {
                throw mx::Exception("Error loading shader program: " + text);
            }
        }
        GLenum error;
        error = glGetError();
        if(error != GL_NO_ERROR){
            throw mx::Exception("OpenGL Error: on ShaderLibary::loadProgram: " + std::to_string(error));
        }
        programs.back()->useProgram();
        
        GLint loc = glGetUniformLocation(programs.back()->id(), "iResolution");
        glUniform2f(loc, win->w, win->h);
        error = glGetError();
        if(error != GL_NO_ERROR) {
            throw mx::Exception("setUniform");
        }
        mx::system_out << "acmx2: Compiled Shader 0: " << text << "\n";
        std::filesystem::path file_path(text);
        std::string name = file_path.stem().string();
        if(!name.empty()) {
            size_t pos = programs.size()-1;
            program_names[pos].name = name;
            program_names[pos].loc = glGetUniformLocation(programs.back()->id(), "alpha");
            program_names[pos].iTime = glGetUniformLocation(programs.back()->id(), "iTime");
            program_names[pos].iMouse = glGetUniformLocation(programs.back()->id(), "iMouse");
            program_names[pos].time_f = glGetUniformLocation(programs.back()->id(), "time_f");
            program_names[pos].iResolution = glGetUniformLocation(programs.back()->id(), "iResolution");

            if(name.find("cache") != std::string::npos) {
                for(int i = 0; i < 4; ++i) {
                    program_names[pos].texture_cache_loc[i] = glGetUniformLocation(programs.back()->id(), std::string("samp" + std::to_string(i+1)).c_str());
                }
            }

#ifdef AUDIO_ENABLED
            program_names[pos].amp = glGetUniformLocation(programs.back()->id(), "amp");
            program_names[pos].amp_untouched = glGetUniformLocation(programs.back()->id(), "uamp");
#endif
        }
    }

    void setUniform(const std::string &name, int value) {
         glUniform1i(program_names[index()].texture_cache_loc[value], value+1);
    }

    void is3D(bool is3d) {
        this->is3d = is3d;
    }

    void loadPrograms(gl::GLWindow *win, const std::string &text) {
        std::fstream file;
        file.open(text + "/index.txt", std::ios::in);
        if(!file.is_open()) {
            throw mx::Exception("acmx2: Could not load index.txt at shader path: " + text);
        }        
        size_t index = 0;
        GLenum error;
        while(!file.eof()) {
            std::string line_data;
            std::getline(file, line_data);
            if(file && !line_data.empty() && std::filesystem::exists(text + "/" + line_data) && line_data.find("material") == std::string::npos) {
                programs.push_back(std::make_unique<gl::ShaderProgram>());
            
                mx::system_out << "acmx2: Compiling Shader: " << index++  << ": [" << line_data << "]\n";
                fflush(stdout);
                fflush(stderr);
                try {
                    if(is3d == true) {
                        if(!programs.back()->loadProgram(win->util.getFilePath("data/vertex.glsl"), text + "/" + line_data)) {
                            throw mx::Exception("acmx2: Error could not load shader: " + line_data);
                        }
                    } else {
                        if(!programs.back()->loadProgram(win->util.getFilePath("data/vert.glsl"), text + "/" + line_data)) {
                            throw mx::Exception("acmx2: Error could not load shader: " + line_data);
                        }
                    }
                } catch(mx::Exception &e) {
                    mx::system_err << "\n";
                    fflush(stdout);
                    fflush(stderr);
                    throw;
                }
                error = glGetError();
                if(error != GL_NO_ERROR) {
                    throw mx::Exception("OpenGL Error loading shader program");
                }
                programs.back()->useProgram();
                //programs.back()->setUniform("proj_matrix", glm::mat4(1.0f));
                //programs.back()->setUniform("mv_matrix", glm::mat4(1.0f));
                GLint loc = glGetUniformLocation(programs.back()->id(), "iResolution");
                glUniform2f(loc, win->w, win->h);
                error = glGetError();
                if(error != GL_NO_ERROR) {
                    throw mx::Exception("setUniform");
                }
            
                fflush(stdout);
                fflush(stderr);
                std::filesystem::path file_path(line_data);
                std::string name = file_path.stem().string();
                if(!name.empty()) {
                    size_t pos = programs.size()-1;
                    program_names[pos].name = name;
                    program_names[pos].loc = glGetUniformLocation(programs.back()->id(), "alpha");
                    program_names[pos].iTime = glGetUniformLocation(programs.back()->id(), "iTime");
                    program_names[pos].iMouse = glGetUniformLocation(programs.back()->id(), "iMouse");
                    program_names[pos].time_f = glGetUniformLocation(programs.back()->id(), "time_f");
                    program_names[pos].iResolution = glGetUniformLocation(programs.back()->id(), "iResolution");
                    if(name.find("cache") != std::string::npos) {
                        for(int i = 0; i < 4; ++i) {
                            program_names[pos].texture_cache_loc[i] = glGetUniformLocation(programs.back()->id(), std::string("samp" + std::to_string(i+1)).c_str());
                        }
                    }
#ifdef AUDIO_ENABLED
                    program_names[pos].amp = glGetUniformLocation(programs.back()->id(), "amp");
                    program_names[pos].amp_untouched = glGetUniformLocation(programs.back()->id(), "uamp");
#endif
                }
           }
        }
        file.close();
    }

    bool isCache() {
        if(library_index < program_names.size() && program_names[library_index].name.find("cache") != std::string::npos)
            return true;
        return false;
    }

    void setIndex(size_t i) {
        if(i < programs.size()) {
            library_index = i;   
            mx::system_out << "acmx2: Set Shader to Index: " << i << " [" << program_names[i].name << "]\n";
            fflush(stdout);
        }
    }
    void inc() {
        if(library_index+1 < programs.size())
            setIndex(library_index+1);
    }
    void dec() {
        if(library_index > 0)
            setIndex(library_index-1);
    }
    size_t index() { return library_index; }

    void useProgram() { 
        programs[index()]->useProgram(); 
    }
    gl::ShaderProgram *shader() { return programs[index()].get(); }

    void update(gl::GLWindow *win) {
        static Uint64 start_time = SDL_GetPerformanceCounter();
        Uint64 now_time = SDL_GetPerformanceCounter();
        double elapsed_time = (double)(now_time - start_time) / SDL_GetPerformanceFrequency();

        if(time_audio == false && time_active) {
            time_f = static_cast<float>(elapsed_time);
        } else {
#ifdef AUDIO_ENABLED
            if(time_audio) {
                time_f += (get_amp() * get_sense());
            }
#endif
        }

        if(std::isnan(time_f) || std::isinf(time_f))
            time_f = 1.0;

        GLuint time_f_loc = program_names[index()].time_f;
        glUniform1f(time_f_loc, time_f);
        GLint loc = program_names[index()].loc;
        glUniform1f(loc, alpha);
        GLuint iTimeLoc = program_names[index()].iTime;
        double currentTime = (float)SDL_GetTicks() / 1000.0f; 
        glUniform1f(iTimeLoc, currentTime);    
        static bool isDragging = false;
        static float clickStartX = 0.0f;
        static float clickStartY = 0.0f;
        GLuint iMouseLoc = program_names[index()].iMouse;
        int mouseX = 0, mouseY = 0;
        Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
        float currentY = static_cast<float>(win->h - mouseY);
        float currentX = static_cast<float>(mouseX);
        if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            if (!isDragging) {
                clickStartX = currentX;
                clickStartY = currentY;
                isDragging = true;
            }
        } else {
            isDragging = false;
        }
        if (isDragging) {
            glUniform4f(iMouseLoc, currentX, currentY, clickStartX, clickStartY);
        } else {
            glUniform4f(iMouseLoc, currentX, currentY, 0.0f, 0.0f);
        }
        GLuint iResolution = program_names[index()].iResolution;
        glUniform2f(iResolution, win->w, win->h);
#ifdef AUDIO_ENABLED
        GLuint amp_i = program_names[index()].amp;
        static float amplitude = 1.0;
        amplitude += (get_amp() * get_sense());
        glUniform1f(amp_i, amplitude);
        GLuint amp_u = program_names[index()].amp_untouched;
        glUniform1f(amp_u, get_amp());
        fflush(stdout);
#endif
    }

    void incTime(float value) {
        if(!time_active) {
            time_f += value;
            mx::system_out << "acmx2: Time step forward: " << time_f << "\n";
            fflush(stdout);
        }
    }

    void decTime(float value) {
        if(!time_active) {
            if(time_f - value > 1.0) {
                time_f -= value;
                mx::system_out << "acmx2: Time step back: " << time_f << "\n";
            } else {
                time_f = 1.0f;
                mx::system_out << "acmx2: Time reset to: " << time_f << "\n";
            }
            fflush(stdout);
        }
    }

    void activeTime(bool t) {
        time_active = t;
        std::string enabled = ((t == true) ? "on" : "off");
        std::cout << "acmx2: active time: " << enabled << "\n";
        fflush(stdout);
    }

    void audioTime(bool t) {
        time_audio = t;
        std::string enabled = ((t == true) ? "on" : "off");
        std::cout << "acmx2: audio time: " << enabled << "\n";
        fflush(stdout);
    }

#ifdef AUDIO_ENABLED
    bool timeActive() const { return time_active; }
    bool timeAudio() const { return time_audio; }
#endif

    void event(SDL_Event &e) {  }

private:
    size_t library_index = 0;
    std::vector<std::unique_ptr<gl::ShaderProgram>> programs;

    struct ProgramData {
        std::string name;
        GLuint loc, iTime, iMouse, time_f, iResolution;
#ifdef AUDIO_ENABLED
        GLuint amp, amp_untouched;
#endif
        GLuint texture_cache_loc[4];
    };
    bool time_audio = false;
    std::unordered_map<int, ProgramData> program_names;
};

struct MXArguments {
    std::string path, filename, ofilename;
    std::string graphic_file;
    int tw = 1280, th = 720;
    int Kbps = 10000;
    int camera_device = 0;
    std::string library = "./filters";
    std::string fragment = "./frag.glsl";
    std::string prefix_path = ".";
    std::string model_file = "cube.mxmod.z";
    int mode = 0;
    int shader_index = 0;
    std::optional<cv::Size> sizev = std::nullopt;
    std::optional<cv::Size> csize = std::nullopt;
    double fps_value = 24.0;
    bool repeat = false;
    std::tuple<int, std::string, int> slib;
    bool full = false;
    bool cache = false;
    int cache_delay = 1;
    bool copy_audio = false;
    bool is3d = false;
#ifdef AUDIO_ENABLED
    bool audio_enabled = false;
    unsigned int audio_channels = 2;
    float audio_sensitivty = 0.25f;
#endif
};

struct FrameData {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    bool isSnapshot = false; 
};


class ACView : public gl::GLObject {
#ifdef AUDIO_ENABLED
    bool audio_is_enabled = false;
#endif
 
public:
    ACView(const MXArguments &args)
        : bit_rate{args.Kbps},
          prefix_path{args.prefix_path},
          filename{args.filename},
          ofilename{args.ofilename},
          graphic{args.graphic_file},
          camera_index{args.camera_device},
          flib{args.slib},
          sizev{args.sizev},
          sizec{args.csize},
          fps{args.fps_value},
          repeat{args.repeat},
          full{args.full},
          frame_cache{4},
          texture_cache{args.cache},
          cache_delay{args.cache_delay},
          copy_audio{args.copy_audio} {
#ifdef AUDIO_ENABLED
        if(args.audio_enabled) {
            if(init_audio(args.audio_channels, args.audio_sensitivty) != 0) {
                mx::system_err << "acmx2: Error could not initalize audio\n";
            } else {
                audio_is_enabled = true;
            }
        }

#endif
        running = true;
        library.is3D(args.is3d);
        is3d_enabled = args.is3d;
        m_file = args.model_file;
    }

    bool is3d_enabled = false;

    
    ~ACView() override {
#ifdef AUDIO_ENABLED
        if(audio_is_enabled) {
            close_audio();
        }
#endif

        stopCaptureThread(); 
        
       if (captureFBO) {
            glDeleteFramebuffers(1, &captureFBO);
            captureFBO = 0;
        }
        if (fboTexture) {
            glDeleteTextures(1, &fboTexture);
            fboTexture = 0;
        }
        if(camera_texture) {
            glDeleteTextures(1, &camera_texture);
        }

        if(texture_cache) {
            glDeleteTextures(4, cache_textures);
        }

        stopWriterThread();
    }
    
    mx::Model cube;
    gl::ShaderProgram fshader;
    std::string m_file;
    
    virtual void load(gl::GLWindow *win) override {
        frame_counter = 0;
        library.is3D(is3d_enabled);
        if(std::get<0>(flib) == 1)
            library.loadPrograms(win, std::get<1>(flib));
        else
            library.loadProgram(win, std::get<1>(flib));
        library.setIndex(std::get<2>(flib));

        if(is3d_enabled && !cube.openModel(win->util.getFilePath("data/" + m_file))) {
            throw mx::Exception("Could not open model: cube.mxmod.z");
        }

        cube.setShaderProgram(library.shader(), "samp");

        if(!fshader.loadProgram(win->util.getFilePath("data/vert.glsl"), win->util.getFilePath("data/framebuffer.glsl"))) {
            throw mx::Exception("Error loading shader");
        }

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            throw mx::Exception("OpenGL error occurred: GL Error: " + std::to_string(error));
        }

        int w = 1280, h = 720;
        int frame_w = w, frame_h = h;

        if(!graphic.empty()) {
            graphic_frame = cv::imread(graphic);
            if(graphic_frame.empty()) {
                throw mx::Exception("Graphics file not found: " + graphic);
            }
            
            w = graphic_frame.cols;
            h = graphic_frame.rows;
            frame_w = w;
            frame_h = h;
            fps = this->fps; 
            
            mx::system_out << "acmx2: Graphics file loaded: " << w << "x" << h << " at FPS: " << fps << "\n";
            fflush(stdout);
            fflush(stderr);
            if(sizev.has_value()) {
                w = sizev.value().width;
                h = sizev.value().height;
                mx::system_out << "acmx2: Resolution stretched to: " << w << "x" << h << "\n";
                fflush(stdout);
                fflush(stderr);
            }

            win->setWindowSize(w, h);
            win->w = w;
            win->h = h;

            if(!ofilename.empty()) {
                if(writer.open(ofilename, w, h, fps, bit_rate)) {
                    mx::system_out << "acmx2: Opened: " << ofilename 
                                   << " for writing at: " << bit_rate 
                                   << " Kbps FPS: " << fps <<"\n";

                    fflush(stdout);
                    fflush(stderr);
                } else {
                    throw mx::Exception("Could not open output video file: " +  ofilename);
                }
            }
        } else if(filename.empty()) {
#ifdef _WIN32
            cap.open(camera_index, cv::CAP_DSHOW);
#else
            cap.open(camera_index);
#endif
            if(!cap.isOpened()) {
                throw mx::Exception("Could not open camera index: " + std::to_string(camera_index));
            }
            if(sizec.has_value()) {
                cap.set(cv::CAP_PROP_FRAME_WIDTH, sizec.value().width);
                cap.set(cv::CAP_PROP_FRAME_HEIGHT, sizec.value().height);
            } else {
                cap.set(cv::CAP_PROP_FRAME_WIDTH, win->w);
                cap.set(cv::CAP_PROP_FRAME_HEIGHT, win->h);
            }
            cap.set(cv::CAP_PROP_FPS, fps);
            w = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
            h = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
            fps = cap.get(cv::CAP_PROP_FPS);
            frame_w = w;
            frame_h = h;
            mx::system_out << "acmx2: Camera opened: " << w << "x" << h << " at FPS: " << fps << "\n";
            fflush(stderr);
            fflush(stdout);

            if(sizev.has_value()) {
                w = sizev.value().width;
                h = sizev.value().height;
                mx::system_out << "acmx2: Resolution stretched to: " << w << "x" << h << "\n";
            }

            win->setWindowSize(w, h);
            win->w = w;
            win->h = h;

            if(!ofilename.empty()) {
                if(writer.open_ts(ofilename, w, h, fps, bit_rate)) {
                    mx::system_out << "acmx2: Opened: " << ofilename 
                                   << " for writing at: " << bit_rate 
                                   << " Kbps FPS: " << fps <<"\n";
                } else {
                    throw mx::Exception("Could not open output video file: " +  ofilename);
                }
            }
        } 
        else if(!filename.empty() && graphic.empty()) {
            cap.open(filename);
            if(!cap.isOpened()) {
                throw mx::Exception("Could not open video file: " + filename);
            }
            w = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
            h = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
            fps = cap.get(cv::CAP_PROP_FPS);

            frame_w = w;
            frame_h = h;

            mx::system_out << "acmx2: Video opened: " << w << "x" << h 
                           << " at FPS: " << fps << "\n";

            if(sizev.has_value()) {
                w = sizev.value().width;
                h = sizev.value().height;
                mx::system_out << "acmx2: Resolution stretched to: " 
                               << w << "x" << h << "\n";
            }

            win->setWindowSize(w, h);
            win->w = w;
            win->h = h;

            if(!ofilename.empty()) {
                if(writer.open(ofilename, w, h, fps, bit_rate)) {
                    mx::system_out << "acmx2: Opened: " << ofilename 
                                   << " for writing at: " << bit_rate << " Kbps\n";
                } else {
                    throw mx::Exception("Could not open output video file: " + ofilename);
                }
            }
            totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
            fflush(stderr);
            fflush(stdout);
        } else if(graphic.empty()  && filename.empty()) {
            throw mx::Exception("Requires input from a file, or camera.");
        }

        library.useProgram();
        if(texture_cache) {
            cv::Mat blankMat = cv::Mat::zeros(frame_h, frame_w, CV_8UC3);
            for(int i = 0; i < 4; ++i) {
                cache_textures[i] = loadTexture(blankMat);
            }
            frame_cache.fill(blankMat);
            mx::system_out << "acmx2: Texture cache initalized.\n";
            fflush(stdout);
        }
        sprite.initSize(win->w, win->h);
        cv::Mat blankMat = cv::Mat::zeros(frame_h, frame_w, CV_8UC3);
        camera_texture = loadTexture(blankMat);
        sprite.setName("samp");
        sprite.initWithTexture(library.shader(), camera_texture, 0, 0, blankMat.cols, blankMat.rows);
        setupCaptureFBO(win->w, win->h);

        if(!graphic.empty())
            win->setWindowTitle("ACMX2 - Graphics Input");
        else if(filename.empty())
            win->setWindowTitle("ACMX2 - Capture Input");
        else
            win->setWindowTitle("ACMX2 - [" + filename + "] 0 seconds, frame 0");

        if(full) {
            win->setFullScreen(true);
        }

        if(writer.is_open() || true /* snapshots possible */) {
            startWriterThread();
        }

        if(filename.empty() && cap.isOpened()) {
            startCaptureThread();
        }
    }

    virtual void draw(gl::GLWindow *win) override {
        if(!running) {
            win->quit();
            return;
        }

        cv::Mat newFrame;
        if(!graphic.empty()) {
            newFrame = graphic_frame.clone();
            cv::flip(newFrame, newFrame, 0);
            frame_counter++; 
        } else if(filename.empty()) {
            std::unique_lock<std::mutex> lock(captureQueueMutex);
            if (!captureQueue.empty()) {
                newFrame = std::move(captureQueue.front());
                captureQueue.pop();
            }
        } else {
            if(!cap.read(newFrame)) {
                if(!filename.empty() && repeat) {
                    mx::system_out << "acmx2: video loop...\n";
                    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                    if(!cap.read(newFrame)) {
                        mx::system_out << "acmx2: cannot read after looping.\n";
                    }
                } else {
                    running = false;
                    finished = true;
                    win->quit();
                    return;
                }
            }
            cv::flip(newFrame, newFrame, 0);
        }

        library.useProgram();

        if(!newFrame.empty()) {
            glActiveTexture(GL_TEXTURE0);
            updateTexture(camera_texture, newFrame);
            if(texture_cache && library.isCache() && (!filename.empty() || !graphic.empty())) { 
                static int counter = 0;
                if(++counter > cache_delay) {
                    frame_cache.push(std::move(newFrame));
                    counter = 0;
                }
                if(frame_cache.isFull()) {
                    for(int i = 0; i < 4; i++) {
                        library.setUniform("samp" + std::to_string(i+1), (i+1));
                        glActiveTexture(GL_TEXTURE1 + i);
                        updateTexture(cache_textures[i], frame_cache.at(i));
                        glBindTexture(GL_TEXTURE_2D, cache_textures[i]);
                    }
                } 
            }
        }          
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glViewport(0, 0, win->w, win->h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        library.useProgram();
        library.update(win);

        if (is3d_enabled) {
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);  
            
            static float rotation = 0.0f;
            rotation += 0.5f; 
            if (rotation > 360.0f) rotation -= 360.0f;
            
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f); 
            glm::vec3 lookDirection;
            const Uint8* keystate = SDL_GetKeyboardState(NULL);

            if (!viewRotationActive) {
                if (keystate[SDL_SCANCODE_W]) {
                    cameraPitch += cameraRotationSpeed * 0.3f;
                    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
                }
                if (keystate[SDL_SCANCODE_S]) {
                    cameraPitch -= cameraRotationSpeed * 0.33f;
                    if (cameraPitch < -89.0f) cameraPitch = -89.0;
                }
                if (keystate[SDL_SCANCODE_A]) {
                    cameraYaw -= cameraRotationSpeed * 0.3f;
                    if (cameraYaw < 0.0f) cameraYaw += 360.0f;
               }
                if (keystate[SDL_SCANCODE_D]) {
                    cameraYaw += cameraRotationSpeed * 0.3f;
                    if (cameraYaw >= 360.0f) cameraYaw -= 360.0f;
                }
            }
            if (viewRotationActive) {
                static float viewRotation = 0.0f;
                viewRotation += 0.3f;
                if (viewRotation > 360.0f) viewRotation -= 360.0f;
                
                float lookX = 0.48f * sin(glm::radians(viewRotation));
                float lookY = 0.48f * sin(glm::radians(viewRotation * 0.7f));
                float lookZ = 0.48f * cos(glm::radians(viewRotation));
                lookDirection = glm::vec3(lookX, lookY, lookZ);
            } else {
                lookDirection.x = cos(glm::radians(cameraPitch)) * cos(glm::radians(cameraYaw));
                lookDirection.y = sin(glm::radians(cameraPitch));
                lookDirection.z = cos(glm::radians(cameraPitch)) * sin(glm::radians(cameraYaw));
                lookDirection = glm::normalize(lookDirection) * 0.48f;
            }

            glm::vec3 cameraTarget = cameraPos + lookDirection;
            glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

            glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
            glm::mat4 projectionMatrix = glm::perspective(
                glm::radians(120.0f),
                static_cast<float>(win->w) / static_cast<float>(win->h),
                0.01f,
                10.0f
            );

            glFrontFace(GL_CW);
            glm::mat4 mvMatrix = viewMatrix * modelMatrix;
            library.shader()->setUniform("mv_matrix", mvMatrix);
            library.shader()->setUniform("proj_matrix", projectionMatrix);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, camera_texture);
            glUniform1i(glGetUniformLocation(library.shader()->id(), "samp"), 0);
            glEnableVertexAttribArray(2);
            for(auto &m : cube.meshes) {
                m.draw();
            }
            glFrontFace(GL_CCW); 
        } else {
            glDisable(GL_DEPTH_TEST);
            library.shader()->setUniform("mv_matrix", glm::mat4(1.0f));
            library.shader()->setUniform("proj_matrix", glm::mat4(1.0f));
            sprite.setShader(library.shader());
            sprite.setName("samp");
            sprite.draw(camera_texture, 0, 0, win->w, win->h);
        }
        bool needWriter = (writer.is_open() || snapshot);
        if (needWriter) {
            std::vector<unsigned char> pixels(win->w * win->h * 4);
            glBindTexture(GL_TEXTURE_2D, fboTexture);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            std::vector<unsigned char> flipped_pixels(win->w * win->h * 4);
            for (int y = 0; y < win->h; ++y) {
                int src_row_start = y * win->w * 4;
                int dest_row_start = (win->h - 1 - y) * win->w * 4;
                std::copy(pixels.begin() + src_row_start,
                          pixels.begin() + src_row_start + (win->w * 4),
                          flipped_pixels.begin() + dest_row_start);
            }

            FrameData fd;
            fd.pixels = std::move(flipped_pixels);
            fd.width  = win->w;
            fd.height = win->h;
            fd.isSnapshot = snapshot;
            snapshot = false;

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                frameQueue.push(std::move(fd));
            }
            queueCondVar.notify_one();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, win->w, win->h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        
        fshader.useProgram();
        fshader.setUniform("mv_matrix", glm::mat4(1.0f));
        fshader.setUniform("proj_matrix", glm::mat4(1.0f));
        sprite.setShader(&fshader);
        sprite.draw(fboTexture, 0, 0, win->w, win->h);
        
        static auto lastUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if(!graphic.empty()) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() >= 100) { 
                double seconds = static_cast<double>(frame_counter) / fps;
                double progress = (static_cast<double>(frame_counter) / static_cast<double>(fps * 10.0)) * 100.0; 
                std::ostringstream stream;
                stream << "ACMX2 - Graphics Mode - " << std::fixed << std::setprecision(1) 
                       << progress << "% Frame: " << frame_counter
                       << " - " << seconds << " seconds";
                win->setWindowTitle(stream.str());
                lastUpdate = now;
                mx::system_out << "acmx2: Processing frame " << frame_counter << "/" << (static_cast<int>(fps * 10.0)) << "\n";
                fflush(stdout);
            }
        } else if(cap.isOpened() && !filename.empty()) {
            
            frame_counter = static_cast<int>(cap.get(cv::CAP_PROP_POS_FRAMES));
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 3) {
                double currentFrame = static_cast<double>(frame_counter);
                double percentage = 0.0;
                double seconds = 0.0;
                if (totalFrames > 0.0) {
                    percentage = (currentFrame / totalFrames) * 100.0;
                }
                if(fps > 0) {
                    seconds = currentFrame / fps;
                }
                std::ostringstream stream;
                stream << "ACMX2 - " << static_cast<int>(percentage) << "% ["
                       << static_cast<int>(currentFrame) << "/"
                       << static_cast<int>(totalFrames) << "] - "
                       << static_cast<int>(seconds) << " seconds - Video Mode";
                win->setWindowTitle(stream.str());
                lastUpdate = now;
            }
        } 
        else if(cap.isOpened() && filename.empty() && writer.is_open()) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 1) {
                auto now = std::chrono::steady_clock::now();
                double elapsedSeconds = std::chrono::duration<double>(now - captureStartTime).count();
                if(fps > 0) {
                    std::ostringstream stream;
                    stream << "ACMX2 - " << elapsedSeconds 
                           << " seconds - [" << frame_counter 
                           << "] - Capture Mode";
                    win->setWindowTitle(stream.str());
                    lastUpdate = now;
                }
            }
        }
        
        if(!graphic.empty() || !filename.empty()) {
            auto m = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count();
            lastFrameTime = now;
            if (fps > 0) {
                int fps_mil = static_cast<int>(1000 / fps);
                if (m < fps_mil) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(fps_mil - m - 1));
                }
            }
        }
    }

    virtual void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_KEYUP:
                switch(e.key.keysym.sym) {
                    case SDLK_UP:
                        library.dec();
                        if(is3d_enabled)
                            cube.setShaderProgram(library.shader());
                        else
                            sprite.setShader(library.shader());
                        break;
                    case SDLK_DOWN:
                        library.inc();
                        if(is3d_enabled)
                            cube.setShaderProgram(library.shader());
                        else
                            sprite.setShader(library.shader());
                        
                        break;
                    case SDLK_z:
                        snapshot = true;
                        break;
#ifdef AUDIO_ENABLED
                    case SDLK_t:
                        library.activeTime(!library.timeActive());
                        break;
#endif
                    case SDLK_f:
                        full = !full;
                        win->setFullScreen(full);
                        break;
#ifdef AUDIO_ENABLED
                    case SDLK_q:
                        library.audioTime(!library.timeAudio());
                        break;
#endif
                    case SDLK_v:
                        viewRotationActive = !viewRotationActive;
                        break;
                }
                break;
            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                    case SDLK_i:
                        library.incTime(0.05f);
                        break;
                    case SDLK_o:
                        library.decTime(0.05f);
                        break;
                }
                break;
        }
        library.event(e);
    }

private:
    unsigned int frame_counter = 0;
    int bit_rate = 25000;
    std::string prefix_path;
    std::string filename, ofilename, graphic;
    int camera_index = 0;
    std::tuple<int, std::string, int> flib;
    std::optional<cv::Size> sizev, sizec;
    ShaderLibrary library;
    Writer writer;
    double fps = 30;
    bool repeat = false;
    bool full = false;
    bool snapshot = false;
    double totalFrames = 0;
    cv::VideoCapture cap;
    cv::Mat graphic_frame;
    gl::GLSprite sprite;
    gl::ShaderProgram shader;
    GLuint camera_texture = 0;
    GLuint captureFBO = 0;
    GLuint fboTexture = 0;
    std::thread writerThread;
    std::atomic<bool> running{false};
    std::queue<FrameData> frameQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondVar;
    std::thread captureThread;
    std::queue<cv::Mat> captureQueue;
    std::mutex captureQueueMutex;
    std::condition_variable captureQueueCondVar;
    std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point captureStartTime;
    FrameCache frame_cache;
    bool texture_cache = false;
    GLuint cache_textures[4];
    int cache_delay = 1;
    std::atomic<bool> finished{false};
    std::atomic<bool> copy_audio{false};
    float cameraYaw = 270.0f;   
    float cameraPitch = 0.0f; 
    const float cameraRotationSpeed = 5.0f; 
    bool viewRotationActive = false; 
    
private:

    void setupCaptureFBO(int width, int height) {
        glGenFramebuffers(1, &captureFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        glGenTextures(1, &fboTexture);
        glBindTexture(GL_TEXTURE_2D, fboTexture);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     width,
                     height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               fboTexture,
                               0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            throw mx::Exception("FBO is not complete.");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    GLuint loadTexture(cv::Mat &frame) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            throw mx::Exception("OpenGL error: glGenTextures() returned " + std::to_string(error));
        }

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        cv::Mat temp;
        cv::cvtColor(frame, temp, cv::COLOR_BGR2RGBA);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, temp.cols, temp.rows,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, temp.ptr());

        error = glGetError();
        if (error != GL_NO_ERROR) {
            throw mx::Exception("OpenGL error: glTexImage2D() returned " + std::to_string(error));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    void updateTexture(GLuint texture, cv::Mat &frame) {
        glBindTexture(GL_TEXTURE_2D, texture);
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);
        glTexSubImage2D(GL_TEXTURE_2D, 
                        0, 0, 0,
                        frame.cols, frame.rows,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        frame.ptr());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void startCaptureThread() {
        if(captureThread.joinable()) {
            return; 
        }
        running = true;
        captureThread = std::thread([this]() {
            while(running) {
                if(!cap.grab()) {
                    running = false;
                    return;
                }
                cv::Mat localFrame;
                if(!cap.retrieve(localFrame)) {
                    mx::system_err << "acmx2: retrieve failed.\n";
                    break;
                }
                cv::flip(localFrame, localFrame, 0);
                {
                    std::lock_guard<std::mutex> lock(captureQueueMutex);
                    captureQueue.push(std::move(localFrame));
                }
                captureQueueCondVar.notify_one();
            }
        });
    }

    void stopCaptureThread() {
        running = false;
        captureQueueCondVar.notify_all();
        if (captureThread.joinable()) {
            captureThread.join();
        }
    }

    void startWriterThread() {
        if (writerThread.joinable()) 
            return;
        running = true;
        writerThread = std::thread([this]() {
            static unsigned int snapshotOffset = 0; 
            captureStartTime = std::chrono::steady_clock::now();
    
            while (running) {
                FrameData fd;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCondVar.wait(lock, [this]() {
                        return !frameQueue.empty() || !running;
                    });

                    if (!running && frameQueue.empty()) {
                        break;
                    }

                    fd = std::move(frameQueue.front());
                    frameQueue.pop();
                }

                if (writer.is_open()) {
                    if(!filename.empty() || !graphic.empty()) { 
                        writer.write(fd.pixels.data());
                    } else {
                        writer.write_ts(fd.pixels.data());
                    }
                }
                if(fd.isSnapshot) {
                    auto now = std::chrono::system_clock::now();
                    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                    std::tm localTime = *std::localtime(&now_c);

                    std::ostringstream oss;
                    oss << std::put_time(&localTime, "%Y.%m.%d-%H.%M.%S");
                    std::string name = prefix_path + "/ACMX2.Snapshot-"
                                     + oss.str() + "-"
                                     + std::to_string(fd.width) + "x"
                                     + std::to_string(fd.height) + "-"
                                     + std::to_string(snapshotOffset++) 
                                     + ".png";

                    png::SavePNG_RGBA(name.c_str(), 
                                      fd.pixels.data(), 
                                      fd.width, fd.height);

                    mx::system_out << "acmx2: Took snapshot: " << name << "\n";
                    fflush(stdout);
                    fflush(stderr);
                }
            }
        });
    }

    void stopWriterThread() {
        bool recording = writer.is_open();
        running = false;
        queueCondVar.notify_all();
        if (writerThread.joinable()) {
            writerThread.join();
        }
        if(recording) {
            writer.close();
            auto now = std::chrono::steady_clock::now();
            double elapsedSeconds = std::chrono::duration<double>(now - captureStartTime).count();
            if(!filename.empty() && fps > 0)
                elapsedSeconds = static_cast<double>(frame_counter) / fps;
            
            mx::system_out << "acmx2: " << " wrote " << elapsedSeconds << " seconds to file: " << ofilename << "\n";
            if(!filename.empty() && repeat == false && copy_audio && finished) {
                transfer_audio(filename, ofilename);
                mx::system_out << "acmx2: copied audio track from: " << filename << " to " << ofilename << "\n";
            }
        }
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(const MXArguments &args) : gl::GLWindow("ACMX2", args.tw, args.th, false) {
        util.path = args.path;
        SDL_Surface *ico = png::LoadPNG(util.getFilePath("data/win-icon.png").c_str());
        if(!ico) {
            throw mx::Exception("Could not load icon: " + util.getFilePath("data/win-icon.png"));
        }
        setWindowIcon(ico);
        SDL_FreeSurface(ico);

        setObject(new ACView(args));
        object->load(this);
        fflush(stdout);
        fflush(stderr);
    }

    ~MainWindow() override {}

    void draw() override {
        glClearColor(0.f, 0.f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }

    
    void event(SDL_Event &e) override {
        
    }
};

const char *message = R"(
-[ Keyboard controls ]- {
    Up arrow - Previous shader
    Down arrow - Next shader
    T - enable/disable time
    I/O - step time if not disabled
    Z - take snapshot
    F - toggle fullscreen
    Q - toggle reactive time (if AUDIO_ENABLED)
    W,A,S,D - Look around in 3D mode
}
)";

template<typename T>
void printAbout(Argz<T> &parser) { 
    std::cout << PROGRAM_NAME << ": " << VERSION_INFO << "\n";
    std::cout << "(C) 2025 " << VERSION_AUTHOR << "\n";
    std::cout << "https://lostsidedead.biz\n";
    std::cout << "Command Line Arguments:\n";
    parser.help(std::cout);
    std::cout << message;
}

int main(int argc, char **argv) {
    fflush(stdout);
    Argz<std::string> parser(argc, argv);    
    parser.addOptionSingle('v', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r',"Resolution WidthxHeight")
          .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight")
          .addOptionSingleValue('d', "Camera Device")
          .addOptionDoubleValue('D', "device", "Device Index")
          .addOptionSingleValue('c', "Camera Resolution")
          .addOptionDoubleValue('C', "camera-res", "Camera Resolution")
          .addOptionSingleValue('i', "Input file")
          .addOptionSingleValue('g', "Input Image")
          .addOptionDoubleValue('G', "graphic", "Input graphics file")
          .addOptionDoubleValue('I', "input", "Input file")
          .addOptionSingleValue('s', "Shader Library Index File")
          .addOptionDoubleValue('S', "shaders", "Shader Library Index File")
          .addOptionSingleValue('f', "Fragment Shader")
          .addOptionDoubleValue('F', "fragment", "Fragment Shader")
          .addOptionSingleValue('h', "Shader Index")
          .addOptionDoubleValue('H', "shader", "Shader Index")
          .addOptionSingleValue('e', "Save Prefix")
          .addOptionDoubleValue('E', "prefix", "Save Prefix")
          .addOptionSingleValue('o', "output file")
          .addOptionDoubleValue('O', "output", "output file")
          .addOptionSingleValue('b', "Bitrate in Kbps")
          .addOptionDoubleValue('B', "bitrate", "Bitrate in Kbps")
          .addOptionSingleValue('u', "frames per second")
          .addOptionDoubleValue('U', "fps", "Frames per second")
          .addOptionSingle('a', "Repeat")
          .addOptionDouble('A', "repeat", "Video repeat")
          .addOptionSingle('n', "fullscreen")
          .addOptionDouble(256, "texture-cache", "Enable texture cache")
          .addOptionDoubleValue(257, "cache-delay", "Cache delay in frames")
          .addOptionDouble(258, "copy-audio", "Copy audio track")
          .addOptionDouble(259, "enable-3d", "Enable 3D cube")
          .addOptionDoubleValue(260, "model", "Model file")
          .addOptionDouble(261, "help", "print help info")
#ifdef AUDIO_ENABLED
          .addOptionSingle('w', "Enable Audio Reactivity")
          .addOptionDouble('W', "enable-audio", "enabled audio reacitivty")
          .addOptionSingleValue('l', "Audio channels")
          .addOptionDoubleValue('L', "channels", "Audio channels")
          .addOptionSingleValue('q', "Audio Sensitivty")
          .addOptionDoubleValue('Q', "sense", "Audio sensitivty")
          .addOptionSingle('y', "Enable Audio Pass-through")
          .addOptionDouble('Y', "pass-through", "Enable audio pass thorugh")
#endif
          .addOptionDouble('N', "fullscreen", "Fullscreen Window (Escape to quit)");

    if(argc == 1) {
        printAbout(parser);
    }
    Argument<std::string> arg;
    MXArguments args;
    int value = 0;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'v':
                case 261:
                    printAbout(parser);
                    exit(EXIT_SUCCESS);
                    break;
                case 'p':
                case 'P':
                    args.path = arg.arg_value;
                    break;
                case 'r':
                case 'R': {
                    auto pos = arg.arg_value.find("x");
                    if(pos == std::string::npos)  {
                        mx::system_err << "Error invalid resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    args.tw = atoi(left.c_str());
                    args.th = atoi(right.c_str());
                    args.sizev = cv::Size(args.tw, args.th);
                }
                break;
                case 'G':
                case 'g':
                    args.graphic_file = arg.arg_value;
                break;
                case 'C':
                case 'c': {
                    auto pos = arg.arg_value.find("x");
                    if(pos == std::string::npos)  {
                        mx::system_err << "Error invalid camera resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    int xw = atoi(left.c_str());
                    int xh = atoi(right.c_str());
                    args.csize = cv::Size(xw, xh);
                }
                break;
                case 'd':
                case 'D':
                    args.camera_device = atoi(arg.arg_value.c_str());
                    break;
                case 's':
                case 'S':
                    args.mode = 1;
                    args.library = arg.arg_value;
                    break;
                case 'F':
                case 'f':
                    args.mode = 0;
                    args.fragment = arg.arg_value;
                    break;
                case 'h':
                case 'H':
                    args.shader_index = atoi(arg.arg_value.c_str());
                    break;
                case 'e':
                case 'E':
                    args.prefix_path = arg.arg_value;
                    break;
                case 'i':
                case 'I':
                    args.filename = arg.arg_value;
                    break;
                case 'o':
                case 'O':
                    args.ofilename = arg.arg_value;
                    break;
                case 'b':
                case 'B':
                    args.Kbps = atoi(arg.arg_value.c_str());
                    break;
                case 'u':
                case 'U':
                    args.fps_value = atof(arg.arg_value.c_str());
                    break;
                case 'a':
                case 'A':
                    args.repeat = true;
                    break;
                case 'n':
                case 'N':
                    args.full = true;
                    break;
                case 256:
                    args.cache = true;
                    mx::system_out << "acmx2: Texture cache enabled.\n";
                    break;
                case 257:
                    args.cache_delay = atoi(arg.arg_value.c_str());
                    mx::system_out << "acmx2: Cache delay set to: " << args.cache_delay << "\n";
                    break;
                case 258:
                    args.copy_audio = true;
                    break;
                case 259:
                    args.is3d = true;
                    mx::system_out << "acmx2: 3D cube enabled.\n";
                    break;
                case 260:
                    args.model_file = arg.arg_value;
                    break;
#ifdef AUDIO_ENABLED
                case 'W':
                case 'w':
                    args.audio_enabled = true;
                    break;
                case 'l':
                case 'L':
                    args.audio_channels = atoi(arg.arg_value.c_str());
                    break;
                case 'Q':
                case 'q':
                    args.audio_sensitivty = atof(arg.arg_value.c_str());
                    break;
                case 'Y':
                case 'y':
                    set_output(true);
                    break;
#endif
            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }    

    if(args.path.empty()) {
        args.path = ".";
        mx::system_out << "acmx2: Path name not provided, using current path...\n";
    }

    try {
        args.slib = std::make_tuple(args.mode, 
                                    (args.mode == 0) ? args.fragment : args.library, 
                                    (args.mode == 0) ? 0 : args.shader_index);

        if(args.filename.empty() && args.cache) {
            throw mx::Exception("Texture cache only works in video mode\n");
        }

        MainWindow main_window(args);
        main_window.loop();
    } 
    catch(const mx::Exception &e) {
        mx::system_err << "acmx2: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    } 
    catch(std::exception &e) {
        mx::system_err << "acmx2: Exception: " << e.what() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    } 
    catch(...) {
        mx::system_err << "acmx2: Unknown exception occurred.\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
