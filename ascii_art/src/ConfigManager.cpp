#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>

ConfigManager::ConfigManager(int argc, char *argv[]) :  output_console(false), output_screen(false), output_file(false), is_config_for_all(false){
    int outCnt = 0, i = 1;
    if (argc < 3) {
        throw std::invalid_argument("Not enough arguments.");
    }
    else if (std::string(argv[1]) == "--conf"){ //global config, for all images
        is_config_for_all = true;
        std::string cfg_path(argv[2]);
        if (!std::filesystem::exists(cfg_path)) {
           throw std::invalid_argument("Config file does not exist.");
        }
        global_cfg = cfg_path;
        i = 3;
    }

    Img current_img;
    for (; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--file"){
            if (i + 1 < argc) {
                output_file_path = argv[++i];
                outCnt++;
                if (outCnt > 1) {
                    throw std::invalid_argument("Multiple output options specified.");
                }
                output_file = true;
            }
            else {
                throw std::invalid_argument("No output file specified.");
            }
            continue;
        }
        else if (arg ==  "--console"){
            output_console = true;
            outCnt++;
            if (outCnt > 1) {
                throw std::invalid_argument("Multiple output options specified.");
            }
            continue;
        }
        else if (arg == "--screen"){
            output_screen = true;
            outCnt++;
            if (outCnt > 1) {
                throw std::invalid_argument("Multiple output options specified.");
            }
            continue;
        }

        if (arg.size() > 5 && (arg.substr(arg.size() - 4) == ".jpg" || arg.substr(arg.size() - 4) == ".png")) {
            if (!std::filesystem::exists(arg)) {
                throw std::invalid_argument("CM constr: Image file does not exist.");
            }
            current_img = Img();
            current_img.image_path = arg;
            images.push_back(current_img);
            image_positions.push_back(args.size());
        }else{
            args.push_back(arg);
        }
    }
    if (image_positions.empty()) {
        throw std::invalid_argument("No image files provided.");
    }
    if (outCnt == 0) {
        throw std::invalid_argument("No output options specified.");
    }
}


bool ConfigManager::parseConfig(const std::string &cfg_path, Img &img) {
    std::ifstream config_file(cfg_path);
    if (!config_file.is_open()) {
        std::cout << "Error: Cannot open config file." << std::endl;
        return false;
    }

    std::string line;
    size_t num;
    int ascii_count = 0;
    while (std::getline(config_file, line)) {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                try{
                    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
                        return !std::isspace(ch);
                    }).base(), value.end());
                    if (key == "brightness") {
                        img.brightness += std::stod(value, &num);
                        if (num < value.size()){
                            throw std::invalid_argument("conversion");
                        }
                    }
                    else if (key == "flip") {
                        if (value == "horizontal") {
                            img.flip_horizontal = !img.flip_horizontal;
                        }
                        else if (value == "vertical") {
                            img.flip_vertical = !img.flip_vertical;
                        }
                    }
                    else if (key == "rotate") {
                        img.rotate = ((img.rotate + std::stoi(value, &num)) % 360 + 360) % 360;
                        if (num < value.size()){
                            throw std::invalid_argument("conversion");
                        }
                    }
                    else if (key == "invert") {
                        img.invert = (value == "true");
                    }
                    else if (key == "scale") {
                        img.scale *= std::stod(value, &num);
                        if (num < value.size()){
                            throw std::invalid_argument("conversion");
                        }
                        if (img.scale < 0.1 || img.scale > 10){
                            throw std::invalid_argument("scale");
                        }
                    }
                    else if (key == "ascii") {
                        ascii_count++;
                        if (ascii_count > 1){
                            throw std::invalid_argument("ascii");
                        }
                        if (!std::filesystem::exists(value)){
                            std::cout << "CM config ascii not exists:" << value <<":x" << std::endl;
                            throw std::invalid_argument("ascii");
                        }
                        std::ifstream charset_file(value);
                        if (!charset_file.is_open()) {
                            std::cout << "Error: Unable to open charset file '" << value << "'." << std::endl;
                            throw std::invalid_argument("ascii");
                        }
                        std::getline(charset_file, img.charset);
                        charset_file.close();
                    }
                    else{
                        throw std::invalid_argument("key");
                    }
                }
                catch(std::invalid_argument& e){
                    std::cout << "CM catch parseCfg, Error: Invalid config file." << std::endl;
                    return false;
                }
            }
        }
    }

    config_file.close();
    return true;
}

void ConfigManager::parseCommandLine() {
    size_t siz = 0;
    if (is_config_for_all){
        for (auto &current_config : images){
            if (!parseConfig(global_cfg, current_config)){
                std::cout << "CM parseCommandLine: Error: Invalid config file." << std::endl;
                return;
            }
        }
    }
    else{
        size_t idx = 0;
        for (auto &current_config : images){
            if (idx+1 >= image_positions.size()){
                siz = args.size();
            }
            else{
                siz = image_positions[idx+1];
            }
            for (size_t i = image_positions[idx]; i < siz; i++) {
                size_t num;
                try{
                    if (args[i] == "--conf"){
                        if (i+1 >= siz){
                            std::cout << "Error: No config file path provided." << std::endl;
                            return;
                        }
                        std::string cfg_path(args[i+1]);
                        if (!std::filesystem::exists(cfg_path)) {
                            std::cout << "CM parseCommandLine: Error: Config file does not exist: " << cfg_path  << std::endl;
                            return;
                        }
                        if (!parseConfig(cfg_path, current_config)) {
                            std::cout << "CM parseCL: Error: Invalid config file." << std::endl;
                            return;
                        }
                        ++i;
                        continue;
                    }
                    else if (args[i] == "--ascii") {

                        if (i + 1 >= siz) {
                            std::cout << "Error: No charset file path provided." << std::endl;
                            return;
                        }
                        std::ifstream charset_file(args[i+1]);
                        if (!charset_file.is_open()) {
                            std::cout << "Error: Unable to open charset file: " << args[i] << "'." << std::endl;
                            return;
                        }
                        std::getline(charset_file, current_config.charset);
                        charset_file.close();
                        ++i;
                        continue;
                    }
                    else if (args[i] == "--brightness" && i + 1 < siz) {
                        current_config.brightness += std::stod(args[i + 1], &num);
                        if (num < args[i + 1].size()){
                            throw std::invalid_argument("brightness conversion");
                        }
                        ++i;
                        continue;
                    }
                    else if (args[i] == "--scale" && i + 1 < siz) {
                        double scale_value = std::stod(args[i + 1], &num);
                        if (num < args[i + 1].size()){
                            throw std::invalid_argument("scale conversion");
                        }
                        if (scale_value < 0 || scale_value > 10){
                            throw std::invalid_argument("scale out of range");
                        }
                        current_config.scale *= scale_value;
                        ++i;
                        continue;
                    }
                    else if (args[i] == "--invert") {
                        current_config.invert = !current_config.invert;
                    }
                    else if (args[i] == "--rotate" && i + 1 < siz) {
                        current_config.rotate = ((current_config.rotate + std::stoi(args[i + 1], &num)) % 360 + 360) % 360;
                        if (num < args[i + 1].size()){
                            throw std::invalid_argument("rotate conversion");
                        }
                        ++i;
                        continue;
                    }
                    else if (args[i] == "--flip-horizontal") {
                        current_config.flip_horizontal = !current_config.flip_horizontal;
                    }
                    else if (args[i] == "--flip-vertical") {
                        current_config.flip_vertical = !current_config.flip_vertical;
                    }
                    else {
                        throw std::out_of_range("CM parseCommandLine: Error: Invalid argument: " + args[i]);
                    }
                }
                catch (std::out_of_range &e){
                    throw std::out_of_range(std::string("err cm pcl: ") + e.what());
                }
            }
            idx++;
        }
    }
}


std::string ConfigManager::getOutputPath() const {
    return output_file_path;
}


std::vector<Img> ConfigManager::getImages() const {
    return images;
}

std::string ConfigManager::getOutputType() const {
    if (output_console){
        return "console";
    }else if (output_screen){
        return "screen";
    }else if (output_file){
        return "file";
    }
    return "";
}
