//
// Created by ousing9 on 2022/3/9.
//

#ifndef STORJ_EMULATOR_SEGMENT_H
#define STORJ_EMULATOR_SEGMENT_H


#include <vector>

#include <boost/uuid/uuid.hpp>

namespace storj
{
    struct segment
    {
        boost::uuids::uuid id;
        boost::uuids::uuid file_id;
        int index;
        std::vector<char> data;

        segment();
        segment(std::vector<char> data);
    };
}

#endif //STORJ_EMULATOR_SEGMENT_H
