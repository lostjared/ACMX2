#define AC_VERSION "0.1.0"
#include<mx.hpp>
#include<argz.hpp>
#include<gl.hpp>
#include<vector>
#include<fstream>
#include<string>
#include<algorithm>
#include<tuple>
#include<unordered_map>
#include<opencv2/opencv.hpp>
#include<filesystem>
#include<chrono>
#include<ctime>

class ShaderLibrary {
    float alpha = 1.0;
    float time_f = 1.0f;
    bool time_active = true;
public:
    ShaderLibrary() = default;
    ~ShaderLibrary() {}
    void loadProgram(gl::GLWindow *win, const std::string text) {
        programs.push_back(std::make_unique<gl::ShaderProgram>());
        if(!programs.back()->loadProgram(win->util.getFilePath("data/vertex.glsl"), text)) {
            throw mx::Exception("Error loading shader program: " + text);
        }
        GLenum error;
        error = glGetError();
        if(error != GL_NO_ERROR){
            throw mx::Exception("OpenGL Error: on ShaderLibary::loadProgram: " + std::to_string(error));
        }
        programs.back()->useProgram();
        programs.back()->setUniform("proj_matrix", glm::mat4(1.0f));
        programs.back()->setUniform("mv_matrix", glm::mat4(1.0f));
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
            program_names[programs.size()-1] = name;
        }
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
            if(file && !line_data.empty() && line_data.find("material") == std::string::npos) {
                programs.push_back(std::make_unique<gl::ShaderProgram>());
                if(!programs.back()->loadProgram(win->util.getFilePath("data/vertex.glsl"), text + "/" + line_data)) {
                    throw mx::Exception("acmx2: Error could not load shader: " + line_data);
                }
                error = glGetError();
                if(error != GL_NO_ERROR) {
                    throw mx::Exception("OpenGL Error loading shader program");
                }
                programs.back()->useProgram();
                programs.back()->setUniform("proj_matrix", glm::mat4(1.0f));
                programs.back()->setUniform("mv_matrix", glm::mat4(1.0f));
                GLint loc = glGetUniformLocation(programs.back()->id(), "iResolution");
                glUniform2f(loc, win->w, win->h);
                error = glGetError();
                if(error != GL_NO_ERROR) {
                    throw mx::Exception("setUniform");
                }
                mx::system_out << "acmx2: Compiled Shader " << index++ << ": " << line_data << "\n";

                std::filesystem::path file_path(line_data);
                std::string name = file_path.stem().string();
                if(!name.empty()) {
                    program_names[programs.size()-1] = name;
                }
           }
        }
        file.close();
    }
    void setIndex(size_t i) {
        if(i < programs.size())
            library_index = i;   
        mx::system_out << "acmx2: Set Shader to Index: " << i << " [" << program_names[i] << "]\n";
        fflush(stdout);
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
    void useProgram() { programs[index()]->useProgram(); }
    gl::ShaderProgram *shader() { return programs[index()].get(); }
    void update() {
        if(time_active) 
            time_f = static_cast<float>(SDL_GetTicks())/1000.0f;

        programs[index()]->setUniform("time_f", time_f);
        GLint loc = glGetUniformLocation(programs.back()->id(), "alpha");
        glUniform1f(loc, alpha);
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
            if(time_f-value > 1.0) {
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
    }

    bool timeActive() const { return time_active; }

private:
    size_t library_index = 0;
    std::vector<std::unique_ptr<gl::ShaderProgram>> programs;
    std::unordered_map<int, std::string> program_names;
};

class ACView : public gl::GLObject {
    cv::Mat frame;
    ShaderLibrary library;
    std::tuple<int, std::string, int> flib;
    std::string prefix_path;
public:
    ACView(int index, std::tuple<int, std::string, int> &slib, std::string prefix) : camera_index{index}, flib{slib}, prefix_path{prefix} {
    }
    ~ACView() override {

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
    }
    virtual void load(gl::GLWindow *win) override {
        if(std::get<0>(flib) == 1)
            library.loadPrograms(win, std::get<1>(flib));
        else 
            library.loadProgram(win, std::get<1>(flib));

        library.setIndex(std::get<2>(flib));
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            throw mx::Exception("OpenGL error occurred: GL Error: " + std::to_string(error));
        }
        cap.open(camera_index);
        if(!cap.isOpened()) {
            throw mx::Exception("Could not open camera at index: " + std::to_string(camera_index));
        }
        cap.set(cv::CAP_PROP_FRAME_WIDTH, win->w);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, win->h);
        int w = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int h = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        mx::system_out << "acmx2: Camera opened at resolution: " << w << "x" << h << "\n";
        camera_width = w;
        camera_height = h;
        if(cap.read(frame)) {
            sprite.initSize(w, h);
            sprite.setName("samp");
            cv::flip(frame, frame, 0);
            camera_texture = loadTexture(frame);
            sprite.initWithTexture(library.shader(), camera_texture, 0, 0, frame.cols, frame.rows);
        }

        setupCaptureFBO(win->w, win->h);
        fflush(stdout);
        fflush(stderr);
    }

    bool snapshot = false;

    virtual void draw(gl::GLWindow *win) override {
        if (cap.isOpened() && cap.read(frame)) {
            cv::flip(frame, frame, 0);
            updateTexture(camera_texture, frame);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glViewport(0, 0, camera_width, camera_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        library.useProgram();
        library.update();
        sprite.draw(camera_texture, 0, 0, frame.cols, frame.rows);
        if(snapshot == true) {
            static unsigned int offset = 0;
            std::vector<unsigned char> pixels(camera_width * camera_height * 4);
            glBindTexture(GL_TEXTURE_2D, fboTexture);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm localTime = *std::localtime(&now_c);
            std::ostringstream oss;
            oss << std::put_time(&localTime, "%Y.%m.%d-%H.%M.%S");
            std::string name = prefix_path + "/ACMX2.Snapshot-" + oss.str() + "-" + std::to_string(offset) + ".png";
            std::vector<unsigned char> flipped_pixels(camera_width * camera_height * 4);
            for (int y = 0; y < camera_height; ++y) {
                int src_row_start = y * camera_width * 4;
                int dest_row_start = (camera_height - 1 - y) * camera_width * 4;
                std::copy(pixels.begin() + src_row_start, pixels.begin() + src_row_start + camera_width * 4, flipped_pixels.begin() + dest_row_start);
            }
            png::SavePNG_RGBA(name.c_str(), flipped_pixels.data(), camera_width, camera_height);
            mx::system_out << "acmx2: Took snapshot: " << name << "\n";
            fflush(stdout);
            fflush(stderr);
            snapshot = false;
            ++offset;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, win->w, win->h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        library.useProgram();
        library.update();
        sprite.draw(fboTexture, 0, 0, win->w, win->h);
      }
    virtual void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_KEYUP:
                switch(e.key.keysym.sym) {
                    case SDLK_UP:
                        library.dec();
                        sprite.setShader(library.shader());
                    break;
                    case SDLK_DOWN:
                        library.inc();
                        sprite.setShader(library.shader());
                    break;
                    case SDLK_z:
                        snapshot = true;
                    break;
                    case SDLK_t:
                        library.activeTime(!library.timeActive());
                    break;
                    case SDLK_i:
                        library.incTime(0.1f);
                        break;
                    case SDLK_o:
                        library.decTime(0.1f);
                }
            break;
        }
    }

    GLuint loadTexture(cv::Mat &frame) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            throw mx::Exception("OpenGL error occurred: glGenTextures: GL Error: " + std::to_string(error));
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.ptr());
        error = glGetError();
        if (error != GL_NO_ERROR) {
            throw mx::Exception("OpenGL error occurred: GL Error: " + std::to_string(error));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    void updateTexture(GLuint texture, cv::Mat &frame) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_RGBA, GL_UNSIGNED_BYTE, frame.ptr());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
private:
    gl::GLSprite sprite;
    gl::ShaderProgram shader;
    cv::VideoCapture cap;
    int camera_index = 0;
    int camera_width = 0;
    int camera_height = 0;
    GLuint camera_texture = 0;
    GLuint captureFBO = 0;   
    GLuint fboTexture = 0;   

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
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(const std::string &prefix, const std::string &path, std::tuple<int, std::string, int> &filename, int camera_device, int tw, int th) : gl::GLWindow("ACMX2", tw, th) {
        util.path = path;
        SDL_Surface *ico = png::LoadPNG(util.getFilePath("data/win-icon.png").c_str());
        if(!ico) {
            throw mx::Exception("Could not load icon: " + util.getFilePath("data/win-icon.png"));
        }
        setWindowIcon(ico);
        SDL_FreeSurface(ico);
        setObject(new ACView(camera_device, filename, prefix));
        object->load(this);
        fflush(stdout);
        fflush(stderr);
    }
    ~MainWindow() override {}

    void draw() override {
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
    void event(SDL_Event &e) override {}
};

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
          .addOptionSingleValue('s', "Shader Library Index File")
          .addOptionDoubleValue('S', "shaders", "Shader Library Index File")
          .addOptionSingleValue('f', "Fragment Shader")
          .addOptionDoubleValue('F', "fragment", "Fragment Shader")
          .addOptionSingleValue('h', "Shader Index")
          .addOptionDoubleValue('H', "shader", "Shader Index")
          .addOptionSingleValue('e', "Save Prefix")
          .addOptionDoubleValue('E', "prefix", "Save Prefix");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1280, th = 720;
    int camera_device = 0;
    std::string library = "./filters";
    std::string fragment = "./frag.glsl";
    std::string prefix_path = ".";
    int mode = 0;
    int shader_index = 0;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'v':
                    std::cout << "acmx2: v" << AC_VERSION << "\n";
                    std::cout << "(C) 2025 LostSideDead.\n";
                    std::cout << "https://lostsidedead.biz\n";
                    parser.help(std::cout);
                    exit(EXIT_SUCCESS);
                    break;
                case 'p':
                case 'P':
                    path = arg.arg_value;
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
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                break;
                case 'd':
                case 'D':
                    camera_device = atoi(arg.arg_value.c_str());
                break;
                case 's':
                case 'S':
                    mode = 1;
                    library = arg.arg_value;
                    break;
                case 'F':
                case 'f':
                    mode = 0;
                    fragment = arg.arg_value;
                    break;
                case 'h':
                case 'H':
                    shader_index = atoi(arg.arg_value.c_str());
                    break;
                case 'e':
                case 'E':
                    prefix_path = arg.arg_value;
                    break;
            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }    
    if(path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        auto t = std::make_tuple(mode, (mode == 0) ? fragment : library, (mode == 0) ? 0 : shader_index);
        MainWindow main_window(prefix_path, path, t, camera_device, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "acmx2: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    } catch(std::exception &e) {
        mx::system_err << "acmx2: Exception: " << e.what() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    } catch(...) {
        mx::system_err << "acmx2: Exception Occoured.\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
