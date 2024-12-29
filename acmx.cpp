#include<mx.hpp>
#include<argz.hpp>
#include<gl.hpp>
#include<vector>
#include<fstream>
#include<string>
#include<algorithm>
#include<opencv2/opencv.hpp>

class ShaderLibrary {
public:
    ShaderLibrary() = default;
    ~ShaderLibrary() {}

    void loadPrograms(gl::GLWindow *win, const std::string &text) {
        std::fstream file;
        file.open(text + "/index.txt", std::ios::in);
        if(!file.is_open()) {
            throw mx::Exception("acmx2: Could not load index.txt at shader path: " + text);
        }        
        size_t index = 0;
        GLenum error;
        while(!file.eof()) {
            std::string line;
            std::getline(file, line);
            if(file && !line.empty() && line.find("material") == std::string::npos) {
                programs.push_back(std::make_unique<gl::ShaderProgram>());
                if(!programs.back()->loadProgram(text + "/vertex.glsl", text + "/" + line)) {
                    throw mx::Exception("acmx2: Error could not load shader: " + line);
                }
                error = glGetError();
                if(error != GL_NO_ERROR) {
                    throw mx::Exception("load program");
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
                mx::system_out << "acmx2: Compiled Shader " << index++ << ": " << line << "\n";
           }
        }
        file.close();
    }

    void setIndex(size_t i) {
        library_index = i;
    }

    void inc() {
        if(library_index+1 < programs.size())
            library_index++;
    }
    void dec() {
        if(library_index > 0)
            library_index--;
    }
    
    size_t index() { return library_index; }
    void useProgram() { programs[index()]->useProgram(); }
    gl::ShaderProgram *shader() { return programs[index()].get(); }
    float alpha = 1.0;
    int alpha_dir = 0;
    void update() {
        programs[index()]->setUniform("time_f", SDL_GetTicks()/1000.0f);
        if(alpha_dir == 0) {
            alpha -= 0.05f;
            if(alpha <= 0.2f) 
                alpha_dir = 1;
        } else {
            alpha += 0.05f;
            if(alpha >= 1.0f)
                alpha_dir = 0;
        }
        GLint loc = glGetUniformLocation(programs.back()->id(), "alpha");
        glUniform1f(loc, alpha);
                
    }
    
private:
    size_t library_index = 0;
    std::vector<std::unique_ptr<gl::ShaderProgram>> programs;
};

class ACView : public gl::GLObject {
    cv::Mat frame;
    ShaderLibrary library;
    std::string flib;
public:
    ACView(int index, const std::string &slib) : camera_index{index}, flib{slib} {
    }
    ~ACView() override {
        if(camera_texture) {
            glDeleteTextures(1, &camera_texture);
        }
    }

    virtual void load(gl::GLWindow *win) override {
        the_font.loadFont(win->util.getFilePath("data/font.ttf"), 16);
        library.loadPrograms(win, flib);
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
        
    }
    virtual void draw(gl::GLWindow *win) override {
        if(cap.isOpened() && cap.read(frame)) {
            cv::flip(frame, frame, 0);
            updateTexture(camera_texture, frame);
            library.useProgram();
            library.update();
            sprite.draw(camera_texture, 0, 0, frame.cols, frame.rows);
        }
        win->text.setColor({ 255, 255, 255, 255 });
        win->text.printText_Solid(the_font, 25, 25, "ACMX2 - https://lostsidedead.biz");
    }
    virtual void event(gl::GLWindow *win, SDL_Event &e) {
        switch(e.type) {
            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                    case SDLK_UP:
                        library.dec();
                        sprite.setShader(library.shader());
                    break;
                    case SDLK_DOWN:
                        library.inc();
                        sprite.setShader(library.shader());
                    break;
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
    mx::Font the_font;
    gl::GLSprite sprite;
    gl::ShaderProgram shader;
    cv::VideoCapture cap;
    int camera_index = 0;
    int camera_width = 0;
    int camera_height = 0;
    GLuint camera_texture = 0;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(const std::string &path, const std::string &filename, int camera_device, int tw, int th) : gl::GLWindow("ACMX2", tw, th) {
        util.path = path;
        setObject(new ACView(camera_device, filename));
        object->load(this);
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
    
    void event(SDL_Event &e) {
        
    }
};

int main(int argc, char **argv) {
    Argz<std::string> parser(argc, argv);    
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r',"Resolution WidthxHeight")
          .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight")
          .addOptionSingleValue('d', "Camera Device")
          .addOptionDoubleValue('D', "device", "Device Index")
          .addOptionSingleValue('s', "Shader Library Index File")
          .addOptionSingleValue('S', "Shader Library Index File");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1280, th = 720;
    int camera_device = 0;
    std::string library = "index.txt";

    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'h':
                case 'v':
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
                    library = arg.arg_value;
                    break;

            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
    
    if(path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        MainWindow main_window(path, library, camera_device, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
    return 0;
}
