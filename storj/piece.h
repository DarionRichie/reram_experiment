//
// Created by ousing9 on 2022/3/9.
//

#ifndef STORJ_EMULATOR_PIECE_H
#define STORJ_EMULATOR_PIECE_H


#include <string>
#include <vector>

#include <boost/uuid/uuid.hpp>

#include "erasure_share.h"

namespace storj
{
    struct piece
    {
        boost::uuids::uuid id;
        boost::uuids::uuid storage_node_id;
        int index;
        boost::uuids::uuid segment_id;
        std::vector<char> data;
        std::vector<erasure_share> erasure_shares;

        piece();
    };
}

#endif //STORJ_EMULATOR_PIECE_H
