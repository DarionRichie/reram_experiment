//
// Created by ousing9 on 2022/3/9.
//

#include <fcntl.h>
#include <unistd.h>

#include <utility>

#include "file.h"
#include "segment.h"
#include "config.h"

storj::file::file() = default;

storj::file::file(const storj::config &cfg) : cfg(cfg)
{}

storj::file::file(std::string filename, const storj::config &cfg) : name(std::move(filename)), cfg(cfg)
{}
