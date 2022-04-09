//
// Created by ousing9 on 2022/3/9.
//

#include <utility>

#include "erasure_share.h"

storj::erasure_share::erasure_share() = default;

storj::erasure_share::erasure_share(std::vector<char> data) : data(std::move(data))
{}
