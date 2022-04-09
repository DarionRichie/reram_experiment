//
// Created by ousing9 on 2022/3/9.
//

#ifndef STORJ_EMULATOR_CONFIG_H
#define STORJ_EMULATOR_CONFIG_H


namespace storj
{
    struct config
    {
        int file_size;
        int segment_size;
        int stripe_size;
        int erasure_share_size;
        int piece_size;
        int k;
        int m;
        int n;

        

        // n -> < 

        static double failure_rate;
        static int total_nodes;
        static double min_churn_per_round;

        config();
        void set_erasure_share_size(int n) {
            erasure_share_size = n;
        }
    };
}

#endif //STORJ_EMULATOR_CONFIG_H
