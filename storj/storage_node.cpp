//
// Created by ousing9 on 2022/3/10.
//

#include "storage_node.h"

storage_node::storage_node() = default;

storage_node::storage_node(const boost::uuids::uuid &id) : id(id)
{}

bool storage_node::operator==(const storage_node &rhs) const
{
    return id == rhs.id;
}

bool storage_node::operator!=(const storage_node &rhs) const
{
    return !(rhs == *this);
}

bool storage_node::operator<(const storage_node &rhs) const
{
    return id < rhs.id;
}

bool storage_node::operator>(const storage_node &rhs) const
{
    return rhs < *this;
}

bool storage_node::operator<=(const storage_node &rhs) const
{
    return !(rhs < *this);
}

bool storage_node::operator>=(const storage_node &rhs) const
{
    return !(*this < rhs);
}
