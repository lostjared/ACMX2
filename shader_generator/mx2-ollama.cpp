#include"mx2-ollama.hpp"


namespace mx {


    std::string ObjectRequest::unescape(const std::string &input) {
        std::string s = input;  
        for (std::size_t pos = s.find(R"(\n)"); pos != std::string::npos; pos = s.find(R"(\n)", pos)) {
            s.replace(pos, 2, "\n");
            pos += 1;
        }

        for (std::size_t pos = s.find(R"(\t)"); pos != std::string::npos; pos = s.find(R"(\t)", pos)) {
            s.replace(pos, 2, "\t");
            pos += 1;
        }

        for (std::size_t pos = s.find(R"(\r)"); pos != std::string::npos; pos = s.find(R"(\r)", pos)) {
            s.replace(pos, 2, "\r");
            pos += 1;
        }

        std::regex unicode_regex(R"(\\u([0-9a-fA-F]{4}))");
        std::smatch match;
        while (std::regex_search(s, match, unicode_regex)) {
            int code = std::stoi(match[1].str(), nullptr, 16);
            char replacement = static_cast<char>(code);
            s.replace(match.position(), match.length(), 1, replacement);
        }
        return s;
    }

    size_t ObjectRequest::WriteCallback(void* contents, size_t size, size_t nmemb, ResponseData* data) {
        size_t total_size = size * nmemb;
        std::string chunk(static_cast<char*>(contents), total_size);
        
        std::istringstream stream(chunk);
        std::string line;
        
        while (std::getline(stream, line)) {
            std::string r = R"REGEX("response"\s*:\s*"([^"]*)")REGEX";
            std::regex re(r);
            std::smatch m;
            
            if (std::regex_search(line, m, re)) {
                std::string unescaped = ObjectRequest::unescape(m[1].str());
                std::cout << unescaped;
                data->shader_stream << unescaped;
                std::cout.flush();
            }
        }
        
        data->response += chunk;
        return total_size;
    }

    std::string ObjectRequest::generateCode() {
        if (host.empty() || model.empty() || filename.empty() || prompt.empty()) {
            std::cerr << "Host, model, filename or prompt not set.\n";
            throw ObjectRequestException("Host, model, filename or prompt not set.");
        }
        const char *shader = R"(#version 330 core
        in vec2 tc; 
        out vec4 color;
        uniform float time_f;
        uniform sampler2D samp;
        uniform float alpha;
        uniform vec2 iResolution;

        void main(void) {
            color = texture(samp, tc);
        }
        )";

        std::ostringstream payload;
        payload << "{"
                << "\"model\":\"" << model << "\","
                << "\"prompt\":\"";
        
        std::ostringstream stream;
        stream << "you are a master GLSL graphics programmer can you take this shader '" 
            << shader 
            << "' and apply these changes to the texture: " 
            << prompt 
            << "\n"
            << "Do not add any other uniform variables. You can create new local variables, but do not create variables and then not define them. Be creative and make it awesome.\n";
        
        std::string prompt = stream.str();
        std::string escaped_prompt;
        for (char c : prompt) {
            if (c == '"') {
                escaped_prompt += "\\\"";
            } else if (c == '\\') {
                escaped_prompt += "\\\\";
            } else if (c == '\n') {
                escaped_prompt += "\\n";
            } else if (c == '\r') {
                escaped_prompt += "\\r";
            } else if (c == '\t') {
                escaped_prompt += "\\t";
            } else {
                escaped_prompt += c;
            }
        }
        
        payload << escaped_prompt;
        payload << "\"}";

        CURL *curl;
        CURLcode res;
        ResponseData response_data;
        
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        
        if (!curl) {
            std::cerr << "Failed to initialize curl\n";
            curl_global_cleanup();
            throw ObjectRequestException("Failed to initialize curl");
        }

        std::string url = "http://" + host + ":11434/api/generate";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        std::string json_data = payload.str();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        
        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            throw ObjectRequestException("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            
            if (response_code != 200) {
                std::cerr << "HTTP error: " << response_code << std::endl;
                std::cerr << "Response: " << response_data.response << std::endl;
            }
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        std::string value = response_data.shader_stream.str();
        size_t start_pos = 0;
        std::string code_text;
        bool found_code = false;
        
        while ((start_pos = value.find("```", start_pos)) != std::string::npos) {
            size_t after_backticks = start_pos + 3;
            size_t line_end = value.find('\n', after_backticks);   
            
            if (line_end != std::string::npos) {
                std::string lang_line = value.substr(after_backticks, line_end - after_backticks);
                if (lang_line.find("glsl") != std::string::npos || lang_line.empty() || 
                    std::all_of(lang_line.begin(), lang_line.end(), ::isspace)) {
                    
                    size_t code_start = line_end + 1;
                    size_t code_end = value.find("```", code_start);            
                    
                    if (code_end != std::string::npos) {
                        code_text = value.substr(code_start, code_end - code_start);
                        found_code = true;
                        break;
                    }
                }
            }
            start_pos += 3;
        }
        
        if (found_code && !code_text.empty()) {
            std::ofstream output(filename);
            output << code_text;
            output.close();
            std::cout << "\nCode outputted to: " << filename << "\n";
        } else {
            std::cout << "\nNo GLSL code block found in response\n";
        }
        return code_text;
    }

}
