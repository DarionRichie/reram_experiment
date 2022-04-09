//
// Created by ousing9 on 2022/3/9.
//

#ifndef STORJ_EMULATOR_FILE_H
#define STORJ_EMULATOR_FILE_H


#include <vector>
#include <string>

#include "config.h"
#include "segment.h"

namespace storj
{
    struct file
    {
        boost::uuids::uuid id;
        std::string name;
        config cfg;
        std::vector<segment> segments;

        file();
        file(const config &cfg);
        file(std::string filename, const config &cfg);
    };
}

#endif //STORJ_EMULATOR_FILE_H
