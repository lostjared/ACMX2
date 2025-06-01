#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <fstream>

void generateCode(const std::string &filename, const std::string &host, const std::string &model, const std::string &code);
std::string unescape(const std::string &input);


std::string unescape(const std::string &input) {
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

void generateCode(const std::string &filename, const std::string &host, const std::string &model, const std::string &code) {
    const char *shader = R"(#version 330 core
    in vec2 TexCoord;
    out vec4 color;
    uniform float time_f;
    uniform sampler2D textTexture;
    uniform float alpha;
    uniform vec2 iResolution;

    void main(void) {
        color = texture(textTexture, TexCoord);
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
           << code 
           << "\n"
           << "Do not add any other uniform variables. Do not create variables and not define them. Be creative and make it awesome.\n";
    payload << stream.str();
    payload << "\"}";

    std::ofstream fout("payload.json");
    if(!fout.is_open()) {
	    std::cerr << "Error writing JSON file.\n";
	    return;
    }
    fout << payload.str();
    fout.close();

    std::ostringstream cmd;
    cmd << "curl -s --no-buffer -X POST http://" << host << ":11434/api/generate "
        << "-H \"Content-Type: application/json\" "
        << "-d @" << "payload.json";
       
    FILE *fptr = popen(cmd.str().c_str(), "r");
    if (!fptr) {
        std::cerr << "Error..\n";
        return;
    }
    char buffer[1024];
    std::ostringstream shader_stream;
    while (fgets(buffer, sizeof(buffer), fptr)) {
        std::string r = R"REGEX("response"\s*:\s*"([^"]*)")REGEX";
        std::regex re(r);
        std::smatch m;
        std::string line(buffer);
        if (std::regex_search(line, m, re)) {		
            std::cout << unescape(m[1].str());
            shader_stream << unescape(m[1].str());
	    fflush(stdout);
        }
    }	 
    pclose(fptr);
    std::string value = shader_stream.str();
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
}

int main(int argc, char **argv) {
    std::string filename = "shader.glsl";
	std::string host = "localhost";
	std::string model = "codellama:7b";
	if(argc >= 2)
		host = argv[1];
	if(argc >= 3)
		model = argv[2];
	if(argc >= 4)
        	filename = argv[3];

	std::cout << "ACMX2 Ai Shader Generator..\n";
	std::cout << "(C) 2025 LostSideDead Software\n";
	std::string line;
	std::cout << "Enter what you want the ACMX2 shader to do: ";
	std::getline(std::cin, line);
	generateCode(filename, host, model, line);
	return 0;
}

