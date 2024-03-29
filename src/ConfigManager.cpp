#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>

ConfigManager::ConfigManager(int argc, char *argv[]) : output_file_path(""), output_console(false), output_screen(false), output_file(false)
{
    int outCnt = 0, i = 1;
    if (argc < 3)
    { // program_name, image, output_type
        throw std::invalid_argument("Not enough arguments.");
    }

    Img current_img;
    for (; i < argc; i++)
    {
        std::string arg(argv[i]);
        if (arg == "--file")
        {
            if (i + 1 < argc)
            {
                output_file_path = argv[++i];
                outCnt++;
                if (outCnt > 1)
                {
                    throw std::invalid_argument("Multiple output options specified.");
                }
                output_file = true;
            }
            else
            {
                throw std::invalid_argument("No output file specified.");
            }
            continue;
        }
        else if (arg == "--console")
        {
            output_console = true;
            outCnt++;
            if (outCnt > 1)
            {
                throw std::invalid_argument("Multiple output options specified.");
            }
            continue;
        }
        else if (arg == "--screen")
        {
            output_screen = true;
            outCnt++;
            if (outCnt > 1)
            {
                throw std::invalid_argument("Multiple output options specified.");
            }
            continue;
        }
        else if (arg == "--image")
        {
            output_image = true;
            outCnt++;
            if (outCnt > 1)
            {
                throw std::invalid_argument("Multiple output options specified.");
            }
            continue;
        }

        if (arg.size() > 5 && (arg.substr(arg.size() - 4) == ".jpg" || arg.substr(arg.size() - 4) == ".png"))
        {
            if (!std::filesystem::exists(arg))
            {
                throw std::invalid_argument("Image file does not exist.");
            }
            current_img = Img();
            current_img.image_path = arg;
            images.push_back(current_img);
            image_positions.push_back(args.size());
        }
        else
        {
            args.push_back(arg);
        }
    }
    if (image_positions.empty())
    {
        throw std::invalid_argument("No image files provided.");
    }
    if (outCnt == 0)
    {
        throw std::invalid_argument("No output options specified.");
    }
}

void ConfigManager::parseConfigFile(const std::string &cfg_path, Img &current_config)
{
    std::ifstream config_file(cfg_path);
    if (!config_file.is_open())
    {
        throw std::invalid_argument("Config file does not exist.");
    }

    std::string line;
    size_t num;
    int ascii_count = 0;
    while (std::getline(config_file, line))
    {
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch)
                                              { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch)
                                { return !std::isspace(ch); })
                       .base(),
                   line.end());

        if (line.empty())
        {
            continue;
        }

        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '='))
        {
            std::string value;
            if (std::getline(is_line, value))
            {
                value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch)
                                         { return !std::isspace(ch); })
                                .base(),
                            value.end());
                if (key == "brightness")
                {
                    current_config.brightness += std::stod(value, &num);
                    if (num < value.size())
                    {
                        throw std::invalid_argument("Invalid brightness value.");
                    }
                }
                else if (key == "flip")
                {
                    if (value == "horizontal")
                    {
                        current_config.flip_horizontal = !current_config.flip_horizontal;
                    }
                    else if (value == "vertical")
                    {
                        current_config.flip_vertical = !current_config.flip_vertical;
                    }
                    else
                    {
                        throw std::invalid_argument("Invalid flip value.");
                    }
                }
                else if (key == "rotate")
                {
                    current_config.rotate = ((current_config.rotate + std::stoi(value, &num)) % 360 + 360) % 360;
                    if (num < value.size())
                    {
                        throw std::invalid_argument("Invalid rotate value.");
                    }
                }
                else if (key == "invert")
                {
                    if (value != "true" && value != "false")
                    {
                        throw std::invalid_argument("Invalid invert value.");
                    }
                    current_config.invert = (value == "true");
                }
                else if (key == "scale")
                {
                    current_config.scale *= std::stod(value, &num);
                    if (num < value.size())
                    {
                        throw std::invalid_argument("Invalid scale value.");
                    }
                }
                else if (key == "ascii")
                {
                    ascii_count++;
                    if (ascii_count > 1)
                    {
                        throw std::invalid_argument("Multiple ascii files.");
                    }
                    if (!std::filesystem::exists(value))
                    {
                        throw std::invalid_argument("Ascii file does not exist.");
                    }
                    std::ifstream charset_file(value);
                    if (!charset_file.is_open())
                    {
                        throw std::invalid_argument("Unable to open charset (ascii) file.");
                    }
                    std::getline(charset_file, current_config.charset);
                    charset_file.close();
                }
                else if (key == "fancy")
                {
                    if (value != "true" && value != "false")
                    {
                        throw std::invalid_argument("Invalid fancy value.");
                    }
                    current_config.fancy = (value == "true");
                }
                else
                {
                    throw std::invalid_argument("Invalid config key.");
                }
            }
            else
            {
                throw std::invalid_argument("No value for some config key.");
            }
        }
    }
    config_file.close();
}

void ConfigManager::parseCommandLine()
{
    size_t siz, idx = 0;
    for (auto &current_config : images)
    {

        if (idx + 1 >= image_positions.size())
        {
            siz = args.size();
        }
        else
        {
            siz = image_positions[idx + 1];
        }

        checkArgs(current_config, 0, image_positions[0]);     // global config loading
        checkArgs(current_config, image_positions[idx], siz); // local config loading
        idx++;
    }
}

void ConfigManager::checkArgs(Img &current_config, size_t min, size_t max)
{
    for (size_t i = min; i < max; i++)
    {
        size_t num;
        if (args[i] == "--conf")
        {
            if (i + 1 >= max)
            {
                throw std::invalid_argument("No config file path provided.");
            }
            std::string cfg_path(args[i + 1]);
            if (!std::filesystem::exists(cfg_path))
            {
                throw std::invalid_argument("Config file does not exist.");
            }
            parseConfigFile(cfg_path, current_config);

            ++i;
            continue;
        }
        else if (args[i] == "--ascii")
        {
            if (i + 1 >= max)
            {
                throw std::invalid_argument("No charset file path provided.");
            }
            std::ifstream charset_file(args[i + 1]);
            if (!charset_file.is_open())
            {
                throw std::invalid_argument("Unable to open charset (ascii) file.");
            }
            std::getline(charset_file, current_config.charset);
            charset_file.close();
            ++i;
            continue;
        }
        else if (args[i] == "--brightness")
        {
            if (i + 1 >= max)
            {
                throw std::invalid_argument("No brightness value provided.");
            }
            current_config.brightness += std::stod(args[i + 1], &num);
            if (num < args[i + 1].size())
            {
                throw std::invalid_argument("Invalid brightness value.");
            }
            ++i;
            continue;
        }
        else if (args[i] == "--scale")
        {
            if (i + 1 >= max)
            {
                throw std::invalid_argument("No scale value provided.");
            }
            double scale_value = std::stod(args[i + 1], &num);
            if (num < args[i + 1].size())
            {
                throw std::invalid_argument("Invalid scale value.");
            }
            current_config.scale *= scale_value;
            ++i;
            continue;
        }
        else if (args[i] == "--invert")
        {
            current_config.invert = !current_config.invert;
        }
        else if (args[i] == "--rotate")
        {
            if (i + 1 >= max)
            {
                throw std::invalid_argument("No rotate value provided.");
            }
            current_config.rotate = ((current_config.rotate + std::stoi(args[i + 1], &num)) % 360 + 360) % 360;
            if (num < args[i + 1].size())
            {
                throw std::invalid_argument("Invalid rotate value.");
            }
            ++i;
            continue;
        }
        else if (args[i] == "--flip-horizontal")
        {
            current_config.flip_horizontal = !current_config.flip_horizontal;
        }
        else if (args[i] == "--flip-vertical")
        {
            current_config.flip_vertical = !current_config.flip_vertical;
        }
        else if (args[i] == "--fancy")
        {
            current_config.fancy = !current_config.fancy;
        }
        else
        {
            throw std::invalid_argument("Invalid argument: " + args[i]);
        }
    }
}

std::string ConfigManager::getOutputPath() const
{
    return output_file_path;
}

std::vector<Img> ConfigManager::getImages() const
{
    return images;
}

std::string ConfigManager::getOutputType() const
{
    if (output_console)
    {
        return "console";
    }
    else if (output_screen)
    {
        return "screen";
    }
    else if (output_file)
    {
        return "file";
    }
    else if (output_image)
    {
        return "image";
    }
    return "";
}
