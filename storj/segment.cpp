//
// Created by ousing9 on 2022/3/9.
//

#include <utility>

#include "segment.h"

storj::segment::segment() = default;

storj::segment::segment(std::vector<char> data) : data(std::move(data))
{}
