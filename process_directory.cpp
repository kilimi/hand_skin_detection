//
// Created by lilita on 2/16/18.
//

#include "process_directory.h"

#include <errno.h>    // errno, ENOENT, EEXIST
#include <sys/stat.h> // stat
#if defined(_WIN32)
#include <direct.h>   // _mkdir
#endif
#include <dirent.h>


std::vector<std::string> ProcessDirectory::readImagesFromFolder(std::string mainPath) {
    std::vector<std::string> imagefiles;
    boost::filesystem::path p (mainPath);

    boost::filesystem::directory_iterator end_itr;

    // cycle through the directory
    for ( boost::filesystem::directory_iterator itr(p); itr != end_itr; ++itr)
    {
        // If it's not a directory, list it. If you want to list directories too, just remove this check.
        if ( boost::filesystem::is_regular_file(itr->status()) )
        {
            boost::filesystem::path file=* itr;
            if (file.extension().string() == ".tiff" || file.extension().string() == ".png" ||
                    file.extension().string() == ".jpeg" || file.extension().string() == ".jpg")
                imagefiles.push_back(file.string());
        }
    }

    return imagefiles;
}

std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string ProcessDirectory::getImageName(std::string path) {
    std::vector<std::string> splits = split(path, '/');
    return splits[splits.size() - 1];

}

/*********************************************************************************************************************/
bool isDirExist(const std::string &path) {
#if defined(_WIN32)
    struct _stat info;
    if (_stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
#endif
}

bool makePath(const std::string &path) {
#if defined(_WIN32)
    int ret = _mkdir(path.c_str());
#else
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);
#endif
    if (ret == 0)
        return true;

    switch (errno) {
        case ENOENT:
            // parent didn't exist, try to create it
        {
            int pos = path.find_last_of('/');
            if (pos == std::string::npos)
#if defined(_WIN32)
                pos = path.find_last_of('\\');
        if (pos == std::string::npos)
#endif
                return false;
            if (!makePath(path.substr(0, pos)))
                return false;
        }
            // now, try to create again
#if defined(_WIN32)
            return 0 == _mkdir(path.c_str());
#else
            return 0 == mkdir(path.c_str(), mode);
#endif

        case EEXIST:
            // done!
            return isDirExist(path);

        default:
            return false;
    }
}

bool ProcessDirectory::createDirectoryIfDoesNotExist(std::string path) {
    return makePath(path);
}
/*********************************************************************************************************************/