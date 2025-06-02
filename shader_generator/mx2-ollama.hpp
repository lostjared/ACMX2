#ifndef __MX2_OLLAMA_HPP__
#define __MX2_OLLAMA_HPP__


#include <curl/curl.h>
#include <iostream>
#include <string>
#include <sstream>
#include <regex>
#include <fstream>
#include <algorithm>    
#include <cctype>


namespace mx {


    struct ResponseData {
        std::string response;
        std::ostringstream shader_stream;
    };

    class ObjectRequestException : public std::exception {
    public:
        explicit ObjectRequestException(const std::string &message) : msg(message) {}
        virtual const char* what() const noexcept override {
            return msg.c_str();
        }
    private:
        std::string msg;
    };  

    class ObjectRequest {
    public:

        explicit ObjectRequest(const std::string &host_ = "localhost", const std::string &model_ = "codellama:7b", const std::string &filename_ = "shader.glsl") : host(host_), model(model_), filename(filename_) {}
        void setHost(const std::string &host_) {
            host = host_;
        }
        void setModel(const std::string &model_) {
            model = model_;
        }
        void setFilename(const std::string &filename_) {
            filename = filename_;
        }
        void setShader(const std::string &shader_) {
            shader = shader_;
        }

        void setPrompt(const std::string &prompt_) {
            prompt = prompt_;
        }

        static std::string unescape(const std::string &input);
        std::string generateCode();
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, ResponseData* data);
    private:
        std::string host;
        std::string model;
        std::string filename;
        std::string shader;
        std::string prompt;
    };



}

#endif