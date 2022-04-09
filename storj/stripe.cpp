//
// Created by ousing9 on 2022/3/9.
//

#include "stripe.h"

#include <utility>

storj::stripe::stripe() = default;

storj::stripe::stripe(std::vector<char> data) : data(std::move(data))
{}
