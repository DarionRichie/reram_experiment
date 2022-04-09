//
// Created by ousing9 on 2022/3/9.
//

#ifndef STORJ_EMULATOR_STRIPE_H
#define STORJ_EMULATOR_STRIPE_H


#include <vector>

#include <boost/uuid/uuid.hpp>

namespace storj
{
    struct stripe
    {
        boost::uuids::uuid id;
        boost::uuids::uuid segment_id;
        int index;
        std::vector<char> data;

        stripe();
        stripe(std::vector<char> data);
    };
}


#endif //STORJ_EMULATOR_STRIPE_H
