#pragma once
#include <filesystem>
#include <string>

class FileProvider
{
public:
    explicit FileProvider(const std::string &baseImg)
    {
        mDirectory = baseImg.substr(0, baseImg.find_last_of('/') + 1).c_str();
        std::string mSelectedFile = baseImg.substr(baseImg.find_last_of('/') + 1);
        mId = getNumberFromStr(mSelectedFile);
    }

    int
    getNumberFromStr(const std::string &filename)
    {
        if (filename.find('i') != std::string::npos) {
            return 0;
        }
        std::string v;
        std::copy_if(filename.begin(), filename.end(), std::back_inserter(v), ::isdigit);
        if (v.empty())
            return 0;
        try {
            return std::stoi(v);  // Attempt to convert to integer
        } catch (const std::invalid_argument &e) {
            std::cerr << "File number conversion error: invalid argument - " << e.what() << '\n';
            return 0;
        } catch (const std::out_of_range &e) {
            std::cerr << "File number conversion error: out of range - " << e.what() << '\n';
            return 0;
        }
    }

    bool isNewFileReady(std::string &outNewFileName)
    {
        const std::filesystem::path sandbox{mDirectory};
        for (auto const &dir_entry : std::filesystem::directory_iterator{sandbox})
        {
            std::string file{dir_entry.path()};
            file = file.substr(file.find_last_of('/') + 1);
            std::string file_ext = file.substr(file.find_last_of(".") + 1);
            std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });

            if (file_ext != "jpg" && file_ext != "jpeg" && file_ext != "jpe" && file_ext != "bmp")
                continue;

            int num = getNumberFromStr(file);
            if (num > mId)
            {
                mId = num;
                mSelectedFile = file;
                outNewFileName = dir_entry.path();
                return true;
            }
        }
        return false;
    }

    std::string mDirectory;
    std::string mSelectedFile;
    int mId;
};