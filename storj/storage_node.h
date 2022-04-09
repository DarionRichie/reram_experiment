//
// Created by ousing9 on 2022/3/10.
//

#ifndef STORJ_EMULATOR_STORAGE_NODE_H
#define STORJ_EMULATOR_STORAGE_NODE_H


#include <boost/uuid/uuid.hpp>

struct storage_node
{
    boost::uuids::uuid id{};

    storage_node();
    storage_node(const boost::uuids::uuid &id);

    bool operator==(const storage_node &rhs) const;
    bool operator!=(const storage_node &rhs) const;
    bool operator<(const storage_node &rhs) const;
    bool operator>(const storage_node &rhs) const;
    bool operator<=(const storage_node &rhs) const;
    bool operator>=(const storage_node &rhs) const;
};


#endif //STORJ_EMULATOR_STORAGE_NODE_H
