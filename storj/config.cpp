//
// Created by ousing9 on 2022/3/9.
//

#include "config.h"

double storj::config::failure_rate = 0.5;
int storj::config::total_nodes = 100;
double storj::config::min_churn_per_round = 1e-10;

storj::config::config() = default;