//
// Created by lilita on 2/16/18.
//

#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>

class ProcessDirectory {

    public:
        ProcessDirectory() {}
        ~ProcessDirectory() {}

        /***
         * Read all images in a folder
         * @param mainPath path to the folder
         * @return std::vector<std::string> path to each image
         */
        std::vector<std::string> readImagesFromFolder(std::string mainPath);

        /***
         * Return image name which is separated by '/' and is the last in the path
         * @param path full path with image name
         * @return std::string image name
         */
        std::string getImageName(std::string path);


        bool createDirectoryIfDoesNotExist(std::string path);

};

