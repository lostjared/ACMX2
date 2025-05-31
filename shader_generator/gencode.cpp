#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <fstream>


std::string convertFormat(const std::string &text) {
    std::string output;
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\n')
            output += ";";
        else if (text[i] == '\t' || text[i] == '\r' || text[i] == '\'' || text[i] == '\"')
            continue;
        else
            output += text[i];
    }
    return output;
} 

std::string unescape(const std::string &input) {
    std::string s = input;  // make a copy, so we can modify in place
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
    return s;
}

void generateCode(std::string host, std::string model, std::string code) {
    const char *shader = R"(#version 330 core
    in vec2 tc;
    out vec4 color;
    uniform float time_f;
    uniform sampler2D samp;
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
           << code 
           << "\n";
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
    bool on = false;
    std::ostringstream shader_stream;
    std::string total = "";
    while (fgets(buffer, sizeof(buffer), fptr)) {
	std::string r = R"REGEX("response"\s*:\s*"([^"]*)"\s*,\s*"done")REGEX";
        std::regex re(r);
	std::smatch m;
    	std::string line(buffer);
	if (std::regex_search(line, m, re)) {

		if(line.find("glsl") != std::string::npos)
			continue;

		if(line.find("```") != std::string::npos) {
			on = !on;
			if(on == false)
				break;
			continue;
		}

		if(on) {
	         	std::cout << unescape(m[1].str());
			shader_stream << unescape(m[1].str());
		}

    	} else {
	       std::cout << line << "\n";
	}
	fflush(stdout);
    }

    pclose(fptr);
    std::ofstream output("shader.glsl");
    output << shader_stream.str() << "\n";
    output.close();
}

int main(int argc, char **argv) {

	std::string host = "localhost";
	std::string model = "codellama:7b";
	if(argc >= 2)
		host = argv[1];
	if(argc >= 3)
		model = argv[2];

	std::cout << "ACMX2 Ai Shader Generator..\n";
	std::cout << "(C) 2025 LostSideDead Software\n";
	std::string line;
	std::cout << "Enter what you want the ACMX2 shader to do: ";
	std::getline(std::cin, line);
	generateCode(host, model, line);
	return 0;
}
