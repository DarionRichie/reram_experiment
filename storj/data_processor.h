//
// Created by ousing9 on 2022/3/9.
//

#ifndef STORJ_EMULATOR_DATA_PROCESSOR_H
#define STORJ_EMULATOR_DATA_PROCESSOR_H


#include <string>
#include <vector>

#include "erasure_share.h"
#include "file.h"
#include "piece.h"
#include "segment.h"
#include "stripe.h"
namespace storj
{
    class data_processor
    {
        config cfg;
    public:
        data_processor(const config &cfg);
        
        std::vector<segment> split_file(file &f);
        std::vector<stripe> split_segment(storj::segment &s) const;
        std::vector<erasure_share> erasure_encode(storj::stripe &s);
        std::vector<piece> merge_to_pieces(std::vector<std::vector<erasure_share>> &s) const;
        std::vector<erasure_share> split_piece(storj::piece &p) const;
        std::vector<stripe> merge_to_stripes(std::vector<std::vector<erasure_share>> &s) const;
        segment merge_to_segment(std::vector<stripe> &stripes) const;
        file merge_to_file(std::vector<segment> &segments) const;
        std::vector<stripe> repair_stripes_from_erasure_shares(const std::vector<std::vector<erasure_share>> &s) const;
    };
}

#endif //STORJ_EMULATOR_DATA_PROCESSOR_H
