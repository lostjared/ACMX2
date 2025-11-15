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
#include <glm/gtc/matrix_transform.hpp>

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
            program_names[pos].iFrame = glGetUniformLocation(programs.back()->id(), "iFrame");
            program_names[pos].iTimeDelta = glGetUniformLocation(programs.back()->id(), "iTimeDelta");
            program_names[pos].iDate = glGetUniformLocation(programs.back()->id(), "iDate");
            program_names[pos].iFrameRate = glGetUniformLocation(programs.back()->id(), "iFrameRate");
            program_names[pos].iMouseClick = glGetUniformLocation(programs.back()->id(), "iMouseClick");
            
            for(int i = 0; i < 4; ++i) {
                std::string channelTime = "iChannelTime[" + std::to_string(i) + "]";
                std::string channelRes = "iChannelResolution[" + std::to_string(i) + "]";
                program_names[pos].iChannelTime[i] = glGetUniformLocation(programs.back()->id(), channelTime.c_str());
                program_names[pos].iChannelResolution[i] = glGetUniformLocation(programs.back()->id(), channelRes.c_str());
            }

            if(name.find("cache") != std::string::npos) {
                for(int i = 0; i < 4; ++i) {
                    program_names[pos].texture_cache_loc[i] = glGetUniformLocation(programs.back()->id(), std::string("samp" + std::to_string(i+1)).c_str());
                }
            }

#ifdef AUDIO_ENABLED
            program_names[pos].amp = glGetUniformLocation(programs.back()->id(), "amp");
            program_names[pos].amp_untouched = glGetUniformLocation(programs.back()->id(), "uamp");
            program_names[pos].iSampleRate = glGetUniformLocation(programs.back()->id(), "iSampleRate");
#endif
        }
    }

    void setFPS(float fps_value) {
        GLuint iFrameRateLoc = program_names[index()].iFrameRate;
        if(iFrameRateLoc != GL_INVALID_INDEX) {
            glUniform1f(iFrameRateLoc, fps_value);
        }
    }

    void setUniform(const std::string &name, int value) {
         glUniform1i(program_names[index()].texture_cache_loc[value], value+1);
    }

    void is3D(bool is3d) {
        this->is3d = is3d;
    }

    void toggleBypass() {
        shader_bypass = !shader_bypass;
        std::string state = shader_bypass ? "disabled" : "enabled";
        mx::system_out << "acmx2: Shader processing " << state << "\n";
        fflush(stdout);
    }

    bool isBypassed() const {
        return shader_bypass;
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
                    program_names[pos].iFrame = glGetUniformLocation(programs.back()->id(), "iFrame");
                    program_names[pos].iTimeDelta = glGetUniformLocation(programs.back()->id(), "iTimeDelta");
                    program_names[pos].iDate = glGetUniformLocation(programs.back()->id(), "iDate");
                    program_names[pos].iFrameRate = glGetUniformLocation(programs.back()->id(), "iFrameRate");
                    program_names[pos].iMouseClick = glGetUniformLocation(programs.back()->id(), "iMouseClick");
                    
                    for(int i = 0; i < 4; ++i) {
                        std::string channelTime = "iChannelTime[" + std::to_string(i) + "]";
                        std::string channelRes = "iChannelResolution[" + std::to_string(i) + "]";
                        program_names[pos].iChannelTime[i] = glGetUniformLocation(programs.back()->id(), channelTime.c_str());
                        program_names[pos].iChannelResolution[i] = glGetUniformLocation(programs.back()->id(), channelRes.c_str());
                    }

                    if(name.find("cache") != std::string::npos) {
                        for(int i = 0; i < 4; ++i) {
                            program_names[pos].texture_cache_loc[i] = glGetUniformLocation(programs.back()->id(), std::string("samp" + std::to_string(i+1)).c_str());
                        }
                    }
#ifdef AUDIO_ENABLED
                    program_names[pos].amp = glGetUniformLocation(programs.back()->id(), "amp");
                    program_names[pos].amp_untouched = glGetUniformLocation(programs.back()->id(), "uamp");
                    program_names[pos].iSampleRate = glGetUniformLocation(programs.back()->id(), "iSampleRate");
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
        static Uint64 last_frame_time = start_time;
        static uint64_t frame_counter = 0;
        
        Uint64 now_time = SDL_GetPerformanceCounter();
        double elapsed_time = (double)(now_time - start_time) / SDL_GetPerformanceFrequency();
        double delta_time = (double)(now_time - last_frame_time) / SDL_GetPerformanceFrequency();
        last_frame_time = now_time;
        frame_counter++;

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
        double currentTime = (double)SDL_GetTicks64() / 1000.0f; 
        glUniform1f(iTimeLoc, currentTime);   
        GLuint iFrameLoc = program_names[index()].iFrame;
        glUniform1i(iFrameLoc, static_cast<int>(frame_counter % INT_MAX));
        GLuint iTimeDeltaLoc = program_names[index()].iTimeDelta;
        glUniform1f(iTimeDeltaLoc, static_cast<float>(delta_time));
        GLuint iDateLoc = program_names[index()].iDate;
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&now_c);
        float year = static_cast<float>(localTime->tm_year + 1900);
        float month = static_cast<float>(localTime->tm_mon + 1);
        float day = static_cast<float>(localTime->tm_mday);
        float seconds = static_cast<float>(localTime->tm_hour * 3600 + 
                                           localTime->tm_min * 60 + 
                                           localTime->tm_sec);
        glUniform4f(iDateLoc, year, month, day, seconds);
        
        GLuint iFrameRateLoc = program_names[index()].iFrameRate;
        if(iFrameRateLoc != GL_INVALID_INDEX) {
            glUniform1f(iFrameRateLoc, 24.0f);
        }
        
        static bool isDragging = false;
        static bool wasClicked = false;
        static float clickStartX = 0.0f;
        static float clickStartY = 0.0f;
        static float lastClickX = 0.0f;
        static float lastClickY = 0.0f;
        
        GLuint iMouseLoc = program_names[index()].iMouse;
        GLuint iMouseClickLoc = program_names[index()].iMouseClick;
        
        int mouseX = 0, mouseY = 0;
        Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
        float currentY = static_cast<float>(win->h - mouseY);
        float currentX = static_cast<float>(mouseX);
        
        if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            if (!isDragging) {
                clickStartX = currentX;
                clickStartY = currentY;
                lastClickX = currentX;
                lastClickY = currentY;
                isDragging = true;
                wasClicked = true;
            }
        } else {
            isDragging = false;
        }
        
        if (isDragging) {
            glUniform4f(iMouseLoc, currentX, currentY, clickStartX, clickStartY);
        } else {
            glUniform4f(iMouseLoc, currentX, currentY, 0.0f, 0.0f);
        }
        
        if(wasClicked && iMouseClickLoc != GL_INVALID_INDEX) {
            glUniform2f(iMouseClickLoc, lastClickX, lastClickY);
        }
        
        GLuint iResolution = program_names[index()].iResolution;
        glUniform2f(iResolution, win->w, win->h);
        
#ifdef AUDIO_ENABLED
        GLuint amp_i = program_names[index()].amp;
        static float amplitude = 1.0;
        float new_amp = amplitude + (get_amp() * get_sense());
        if (std::isnan(new_amp) || std::isinf(new_amp) || new_amp > 1e6f) {
            amplitude = 1.0f;
        } else {
            amplitude = new_amp;
        }
        glUniform1f(amp_i, amplitude);
        GLuint amp_u = program_names[index()].amp_untouched;
        glUniform1f(amp_u, get_amp());
        GLuint iSampleRateLoc = program_names[index()].iSampleRate;
        if(iSampleRateLoc != GL_INVALID_INDEX) {
            glUniform1f(iSampleRateLoc, 44100.0f);
        }
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
        mx::system_out << "acmx2: active time: " << enabled << "\n";
        fflush(stdout);
    }

    void audioTime(bool t) {
        time_audio = t;
        std::string enabled = ((t == true) ? "on" : "off");
        mx::system_out << "acmx2: audio time: " << enabled << "\n";
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
        GLuint iFrame;
        GLuint iTimeDelta;
        GLuint iDate; 
        GLuint iChannelTime[4];
        GLuint iChannelResolution[4];
        GLuint iSampleRate;
        GLuint iFrameRate; 
        GLuint iMouseClick;
    };
    bool time_audio = false;
    std::unordered_map<int, ProgramData> program_names;
    bool shader_bypass = false;
};

struct MXArguments {
    std::string path, filename, ofilename;
    std::string graphic_file;
    int audio_input = -1, audio_output = -1;
    int tw = 1280, th = 720;
    std::string crf = "23";
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
    int audio_input_device;
    int audio_output_device;
#endif
    bool isPaused = false;
    bool isFrozen = false;
    GLuint pboIds[2] = {0, 0};  
    int pboIndex = 0;
    int pboNextIndex = 1;
public:
    ACView(const MXArguments &args)
        : crf{args.crf},
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
        audio_input_device = args.audio_input;
        audio_output_device = args.audio_output;
        if(args.audio_enabled) {
            if(init_audio(args.audio_channels, args.audio_sensitivty, audio_input_device, audio_output_device) != 0) {
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
    
        if (pboIds[0] && writer.is_open() && win_w > 0 && win_h > 0) {
            for (int i = 0; i < 2; i++) {
                glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[i]);
                GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
                
                if (src) {
                    std::vector<unsigned char> pixels(win_w * win_h * 4);
                    std::memcpy(pixels.data(), src, pixels.size());
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                    
                    std::vector<unsigned char> flipped_pixels(win_w * win_h * 4);
                    for (int y = 0; y < win_h; ++y) {
                        int src_row_start  = y * win_w * 4;
                        int dest_row_start = (win_h - 1 - y) * win_w * 4;
                        std::copy(pixels.begin() + src_row_start,
                                  pixels.begin() + src_row_start + (win_w * 4),
                                  flipped_pixels.begin() + dest_row_start);
                    }

                    FrameData fd;
                    fd.pixels = std::move(flipped_pixels);
                    fd.width  = win_w;
                    fd.height = win_h;
                    fd.isSnapshot = false;

                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        frameQueue.push(std::move(fd));
                    }
                    queueCondVar.notify_one();
                }
                glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            }
            
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        
        stopWriterThread();
        
        
        if (pboIds[0]) {
            glDeleteBuffers(2, pboIds);
            pboIds[0] = pboIds[1] = 0;
        }

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
            camera_texture = 0;
        }

        if(texture_cache) {
            glDeleteTextures(4, cache_textures);
            for(int i = 0; i < 4; i++) {
                cache_textures[i] = 0;
            }
        }
    }
    
    mx::Model cube;
    gl::ShaderProgram fshader, fshader3d;
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
         if(!fshader3d.loadProgram(win->util.getFilePath("data/vertex.glsl"), win->util.getFilePath("data/framebuffer.glsl"))) {
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
            
            SDL_SetWindowPosition(win->getWindow(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

            if(!ofilename.empty()) {
                if(writer.open(ofilename, w, h, fps, crf.c_str())) {
                    mx::system_out << "acmx2: Opened: " << ofilename 
                                   << " for writing at: " << crf 
                                   << " CRF FPS: " << fps <<"\n";

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
            cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
            if(sizec.has_value()) {
                cap.set(cv::CAP_PROP_FRAME_WIDTH, sizec.value().width);
                cap.set(cv::CAP_PROP_FRAME_HEIGHT, sizec.value().height);
            } else {
                cap.set(cv::CAP_PROP_FRAME_WIDTH, win->w);
                cap.set(cv::CAP_PROP_FRAME_HEIGHT, win->h);
            }
            cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
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

            SDL_SetWindowPosition(win->getWindow(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

            if(!ofilename.empty()) {
                if(writer.open_ts(ofilename, w, h, fps, crf.c_str())) {
                    mx::system_out << "acmx2: Opened: " << ofilename 
                                   << " for writing at: " << crf 
                                   << " CRF FPS: " << fps <<"\n";
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
            totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT); 

            frame_w = w;
            frame_h = h;

            mx::system_out << "acmx2: Video opened: " << w << "x" << h 
                           << " at FPS: " << fps 
                           << " Total Frames: " << totalFrames << "\n"; 
            fflush(stdout);
            fflush(stderr);

            if(sizev.has_value()) {
                w = sizev.value().width;
                h = sizev.value().height;
                mx::system_out << "acmx2: Resolution stretched to: " 
                            << w << "x" << h << "\n";
                fflush(stdout);
                fflush(stderr);
            }

            win->setWindowSize(w, h);
            win->w = w;
            win->h = h;
            
            SDL_SetWindowPosition(win->getWindow(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

            if(!ofilename.empty()) {
                if(writer.open(ofilename, w, h, fps, crf.c_str())) {
                    mx::system_out << "acmx2: Opened: " << ofilename 
                                   << " for writing at: " << crf << " CRF\n";
                    fflush(stdout);
                    fflush(stderr);
                } else {
                    throw mx::Exception("Could not open output video file: " + ofilename);
                }
            }
        } else if(graphic.empty() && filename.empty()) {
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
        glGenBuffers(2, pboIds);
        size_t pboSize = win->w * win->h * 4;
        for (int i = 0; i < 2; i++) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[i]);
            glBufferData(GL_PIXEL_PACK_BUFFER, pboSize, nullptr, GL_STREAM_READ);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
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

    cv::Mat newFrame;

    virtual void draw(gl::GLWindow *win) override {
        if(!running) {
            win->quit();
            return;
        }

        if(!isPaused && !isFrozen) {
            if(!graphic.empty()) {
                newFrame = graphic_frame.clone();
                cv::flip(newFrame, newFrame, 0);
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
                if(!newFrame.empty())
                    cv::flip(newFrame, newFrame, 0);
            }
        }
        if(library.isBypassed()) {
            if(is3d_enabled) {
                fshader3d.useProgram();
            } else {
                fshader.useProgram();
            }
        } else {
            library.useProgram();
        }
        if(!isFrozen && !newFrame.empty()) {
            glActiveTexture(GL_TEXTURE0);
            updateTexture(camera_texture, newFrame);
            if(texture_cache && library.isCache() && (!filename.empty() || !graphic.empty())) { 
                static int counter = 0;
                if(++counter > cache_delay) {
                    frame_cache.push(std::move(newFrame));
                    counter = 0;
                }
                if(frame_cache.isFull()) {
                    for(int i = 0; i < 4; ++i) {
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

        if(!isFrozen && !library.isBypassed()) {
            library.useProgram();
            library.update(win);
            library.setFPS(static_cast<float>(fps)); 
        }

        if (is3d_enabled) {
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);  
            
            static float rotation = 0.0f;
            rotation = fmod(rotation + 0.5f, 360.0f);
            
            const Uint8* keystatex = SDL_GetKeyboardState(NULL);
            if (!oscillateScale) {
                if (keystatex[SDL_SCANCODE_EQUALS] || keystatex[SDL_SCANCODE_KP_PLUS]) {
                    cameraDistance += 0.01f;
                    if (cameraDistance > 0.4f) cameraDistance = 0.4f;
                    mx::system_out << "acmx2: cameraDistance increased: " << cameraDistance << "\n";
                    fflush(stdout);
                }
                if (keystatex[SDL_SCANCODE_MINUS] || keystatex[SDL_SCANCODE_KP_MINUS]) {
                    cameraDistance -= 0.01f;                 
                    if (cameraDistance < -0.2f) cameraDistance = -0.2f;
                    mx::system_out << "acmx2: cameraDistance decreased: " << cameraDistance << "\n";
                    fflush(stdout);
                }
            }
            static float t = 0.0f;
            float oscOffset = 0.0f;
            if (oscillateScale) {
                t += 0.02f;
                oscOffset = 0.3f * std::sin(t);
            }

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            glm::vec3 cameraPosBase = glm::vec3(0.0f, 0.0f, 0.0f);
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
                    cameraYaw = fmod(cameraYaw + 360.0f, 360.0f);
                }
                if (keystate[SDL_SCANCODE_D]) {
                    cameraYaw += cameraRotationSpeed * 0.3f;
                    cameraYaw = fmod(cameraYaw, 360.0f);
                }
            }
            if (viewRotationActive) {
                static float viewRotation = 0.0f;
                viewRotation = fmod(viewRotation + 0.3f, 360.0f);
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

            float finalOffset = oscillateScale ? oscOffset : cameraDistance;
            glm::vec3 cameraPos = cameraPosBase - glm::normalize(lookDirection) * finalOffset;
            glm::vec3 cameraTarget = cameraPos + lookDirection;
            glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
            glm::mat4 projectionMatrix = glm::perspective(
                glm::radians(120.0f),
                static_cast<float>(win->w) / static_cast<float>(win->h),
                0.01f,
                10.0f
            );
            glm::mat4 mvMatrix = viewMatrix * modelMatrix;
            gl::ShaderProgram *activeShader;
            if(library.isBypassed()) {
                activeShader = &fshader3d;
            } else {
                activeShader = library.shader();
            }
            activeShader->setUniform("mv_matrix", mvMatrix);
            activeShader->setUniform("proj_matrix", projectionMatrix);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, camera_texture);
            glUniform1i(glGetUniformLocation(activeShader->id(), "samp"), 0);
            glEnableVertexAttribArray(2);
            
            if(!library.isBypassed()) {
                cube.setShaderProgram(activeShader);
            } else {
                cube.setShaderProgram(&fshader3d);
            }
            
            for(auto &m : cube.meshes) {
                m.draw();
            }
            glFrontFace(GL_CCW); 
        } else {
            glDisable(GL_DEPTH_TEST);
            gl::ShaderProgram *activeShader;
            if(library.isBypassed()) {
                activeShader = &fshader;
            } else {
                activeShader = library.shader();
            }
            activeShader->setUniform("mv_matrix", glm::mat4(1.0f));
            activeShader->setUniform("proj_matrix", glm::mat4(1.0f));
            sprite.setShader(activeShader);
            sprite.setName("samp");
            sprite.draw(camera_texture, 0, 0, win->w, win->h);
        }
        bool needWriter = (writer.is_open() || snapshot) && !isFrozen;

        static auto lastEncodedFrameTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();   

        if (needWriter) {
            double targetMsPerFrame = (fps > 0.0) ? (1000.0 / fps) : 0.0;
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastEncodedFrameTime).count();

            bool allowSnapshot   = snapshot;
            bool allowVideoFrame = (fps <= 0.0) || (elapsedMs >= targetMsPerFrame);

            if (allowSnapshot || allowVideoFrame) {
                lastEncodedFrameTime = now;
                glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[pboIndex]);
                glBindTexture(GL_TEXTURE_2D, fboTexture);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0); 
                glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[pboNextIndex]);
                GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
                
                if (src) {
                    std::vector<unsigned char> pixels(win->w * win->h * 4);
                    std::memcpy(pixels.data(), src, pixels.size());
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                    
                    
                    std::vector<unsigned char> flipped_pixels(win->w * win->h * 4);
                    for (int y = 0; y < win->h; ++y) {
                        int src_row_start  = y * win->w * 4;
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
                        if (frameQueue.size() > 30) {
                        frames_dropped++;
                        if (frames_dropped % 10 == 0) { 
                            mx::system_err << "acmx2: Total frames dropped: " << frames_dropped << "\n";
                            fflush(stderr);
                            fflush(stdout);
                        }
                        frameQueue.pop();
                    }
                        frameQueue.push(std::move(fd));
                    }
                    queueCondVar.notify_one();
                }
                
                glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                
                
                pboIndex = (pboIndex + 1) % 2;
                pboNextIndex = (pboNextIndex + 1) % 2;
            }
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
        

        if (!graphic.empty()) {
            if (writer.is_open() &&
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() >= 250) {

                double elapsedSeconds = writer.get_duration();
                int64_t temp_frames   = writer.get_frame_count();

                std::ostringstream stream;
                stream << "ACMX2 - Graphics Mode - "
                       << std::fixed << std::setprecision(1)
                       << elapsedSeconds << " seconds"
                       << " [" << temp_frames << " frames]";
                win->setWindowTitle(stream.str());
                lastUpdate = now;
            } else if (!writer.is_open()) {
                win->setWindowTitle("ACMX2 - Graphics Mode");
            }

        } else if (cap.isOpened() && !filename.empty()) {
            frame_counter = static_cast<unsigned int>(cap.get(cv::CAP_PROP_POS_FRAMES));
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 3) {

                double currentFrame = static_cast<double>(frame_counter);
                double percentage   = 0.0;
                double seconds      = 0.0;

                if (totalFrames <= 0.0) {
                    totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
                }
                if (totalFrames > 0.0) {
                    percentage = (currentFrame / totalFrames) * 100.0;
                }

                double displaySeconds = seconds;
                int64_t displayFrames = static_cast<int64_t>(currentFrame);

                if (writer.is_open()) {
                    double recordedSeconds = writer.get_duration();
                    int64_t recordedFrames = writer.get_frame_count();
                    if (recordedSeconds > 0.0) {
                        displaySeconds = recordedSeconds;
                        displayFrames  = recordedFrames;
                    } else if (fps > 0.0) {
                        seconds = currentFrame / fps;
                        displaySeconds = seconds;
                    }
                } else if (fps > 0.0) {
                    seconds = currentFrame / fps;
                    displaySeconds = seconds;
                }

                std::ostringstream stream;
                stream << "ACMX2 - " << static_cast<int>(percentage) << "% ["
                       << static_cast<int>(displayFrames) << "/"
                       << static_cast<int>(totalFrames) << "] - "
                       << static_cast<int>(displaySeconds) << " seconds - Video Mode";
                win->setWindowTitle(stream.str());
                lastUpdate = now;
            }

        } else if (cap.isOpened() && filename.empty() && writer.is_open()) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 1) {

                double elapsedSeconds = writer.get_duration();
                int64_t temp_frames   = writer.get_frame_count();

                std::ostringstream stream;
                stream << "ACMX2 - Capture Mode - "
                       << std::fixed << std::setprecision(1)
                       << elapsedSeconds << " seconds"
                       << " [" << temp_frames << " frames]";
                win->setWindowTitle(stream.str());
                lastUpdate = now;
            }
        }

        if (!graphic.empty() || !filename.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count();
            if (fps > 0.0) {
                int target_ms = static_cast<int>(1000.0 / fps);
                int sleep_ms = target_ms - static_cast<int>(elapsed);
                if (sleep_ms > 0 && sleep_ms < target_ms) {  
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                }
            }
            lastFrameTime = std::chrono::steady_clock::now();
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
                    case SDLK_SPACE:
                        library.toggleBypass();
                        break;
                    case SDLK_p:
                        if(!filename.empty() || !graphic.empty()) {
                            isPaused = !isPaused;
                            mx::system_out << "acmx2: paused: " << ((isPaused == true) ? "enabled" : "disabled") << "\n";
                            fflush(stdout);
                            fflush(stderr);
                        }
                        break;
                    case SDLK_l:
                        if(!filename.empty() || !graphic.empty()) {
                            isFrozen = !isFrozen;
                            mx::system_out << "acmx2: frozen: " << ((isFrozen == true) ? "enabled" : "disabled") << "\n";
                            fflush(stdout);
                            fflush(stderr);
                        }
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
                        mx::system_out << "acmx2: View rotation: " << (viewRotationActive ? "enabled" : "disabled") << "\n";
                        fflush(stdout);
                        break;
                    case SDLK_x:
                        cameraDistance = 0.0f;
                        mx::system_out << "acmx2: Camera distance reset\n";
                        fflush(stdout);
                        break;
                    case SDLK_o:   
                        oscillateScale = !oscillateScale;
                        mx::system_out << "acmx2: Scale oscillation "
                                       << (oscillateScale ? "enabled" : "disabled") << "\n";
                        fflush(stdout);
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
    unsigned int written_frame_counter = 0; 
    std::string crf = "23";
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
    float modelScale = 1.0f;
    float scaleSpeed = 0.05f;
    bool oscillateScale = false;
    float cameraDistance = 0.0f;
private:
    std::atomic<uint64_t> frames_dropped{0};
    int win_w = 0;
    int win_h = 0;
    
    void flushPBOs(gl::GLWindow *win) {
        if (!pboIds[0]) return;
           for (int i = 0; i < 2; i++) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[i]);
            GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
            
            if (src) {
                std::vector<unsigned char> pixels(win->w * win->h * 4);
                std::memcpy(pixels.data(), src, pixels.size());
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                
                std::vector<unsigned char> flipped_pixels(win->w * win->h * 4);
                for (int y = 0; y < win->h; ++y) {
                    int src_row_start  = y * win->w * 4;
                    int dest_row_start = (win->h - 1 - y) * win->w * 4;
                    std::copy(pixels.begin() + src_row_start,
                              pixels.begin() + src_row_start + (win->w * 4),
                              flipped_pixels.begin() + dest_row_start);
                }

                FrameData fd;
                fd.pixels = std::move(flipped_pixels);
                fd.width  = win->w;
                fd.height = win->h;
                fd.isSnapshot = false;

                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    frameQueue.push(std::move(fd));
                }
                queueCondVar.notify_one();
            }
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }
    }

    void setupCaptureFBO(int width, int height) {
        win_w = width;
        win_h = height;
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
        cv::Mat temp;
        cv::cvtColor(frame, temp, cv::COLOR_BGR2RGBA);  
        glTexSubImage2D(GL_TEXTURE_2D, 
                        0, 0, 0,
                        temp.cols, temp.rows,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        temp.ptr());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void startCaptureThread() {
        if(captureThread.joinable()) {
            return; 
        }
        running = true;
        captureThread = std::thread([this]() {
            try {
                auto lastCapture = std::chrono::steady_clock::now();
                auto frameInterval = std::chrono::microseconds(static_cast<int64_t>(1000000.0 / fps));
                
                while(running) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastCapture);
                    if (elapsed < frameInterval) {
                        std::this_thread::sleep_for(frameInterval - elapsed);
                        continue;
                    }
                    
                    lastCapture = now;
                    
                    cv::Mat localFrame;
                    if(!cap.read(localFrame)) {
                        mx::system_err << "acmx2: camera read failed.\n";
                        running = false;
                        break;
                    }
                    if(localFrame.empty()) {
                        continue;
                    }
                    cv::flip(localFrame, localFrame, 0);
                    {
                        std::lock_guard<std::mutex> lock(captureQueueMutex);
                        while(captureQueue.size() >= 2) {
                            captureQueue.pop(); 
                        }
                        captureQueue.push(std::move(localFrame));
                    }
                    captureQueueCondVar.notify_one();
                }
            } catch(const std::exception &e) {
                mx::system_err << "acmx2: Capture thread exception: " << e.what() << "\n";
                running = false;
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
        written_frame_counter = 0;
        writerThread = std::thread([this]() {
        try {
            static uint64_t snapshotOffset = 0; 
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
                    written_frame_counter++;
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
        } catch(const std::exception &e) {
            mx::system_err << "acmx2: Writer thread exception: " << e.what() << "\n";
            running = false;
            fflush(stderr);
            fflush(stdout);
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
            int64_t final_frame_count = writer.get_frame_count();
            double elapsedSeconds = writer.get_duration(); 
            mx::system_out << "acmx2: " << " wrote " << elapsedSeconds << " seconds (" << final_frame_count << " frames) to file: " << ofilename << "\n";
            if(!filename.empty() && repeat == false && copy_audio && finished) {
                transfer_audio(filename, ofilename);
                mx::system_out << "acmx2: copied audio track from: " << filename << " to " << ofilename << "\n";
            }
            fflush(stdout);
            fflush(stderr);
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
    Space -  Enable/Disable Processing
    L - Enable/Disable video freeze (Video/Image Modes)
    P - Enable/Disable pause video (Video/Image Modes)
    T - enable/disable time
    I/O - step time if not disabled
    Z - take snapshot
    F - toggle fullscreen
    Q - toggle reactive time (if AUDIO_ENABLED)
    3D mode:
    W,A,S,D - Look around 
    O - Oscillation Toggle
    +, - increase / decrease model scale
}
)";

template<typename T>
void printAbout(Argz<T> &parser) { 
    mx::system_out << PROGRAM_NAME << ": " << VERSION_INFO << "\n";
    mx::system_out << "(C) 2025 " << VERSION_AUTHOR << "\n";
    mx::system_out << "https://lostsidedead.biz\n";
    mx::system_out << "Command Line Arguments:\n";
    parser.help(mx::system_out);
    mx::system_out << message;
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
          .addOptionSingleValue('b', "Bitrate in CRF")
          .addOptionDoubleValue('B', "bitrate", "Bitrate in CRF")
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
          .addOptionDoubleValue(300, "audio-input", "Audio input device")
          .addOptionDoubleValue(301, "audio-output", "Audio output device")
          .addOptionDouble(302, "list-devices", "list audio devices")
#endif
          .addOptionDouble('N', "fullscreen", "Fullscreen Window (Escape to quit)");

    if(argc == 1) {
        printAbout(parser);
        exit(EXIT_SUCCESS);
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
                    args.crf = arg.arg_value;
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
                 case 300:
                    if(arg.arg_value == "default")
                        args.audio_input = -1;
                    else
                        args.audio_input = atoi(arg.arg_value.c_str());
                break;
                case 301:
                    if(arg.arg_value == "default") 
                        args.audio_output= -1;
                    else
                        args.audio_output = atoi(arg.arg_value.c_str());
                break;
                case 302:
                    list_audio_devices();
                    exit(EXIT_SUCCESS);
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
