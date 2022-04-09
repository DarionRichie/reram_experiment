//
// Created by ousing9 on 2022/3/9.
//

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include<time.h>
#include "jerasure.h"
#include "cauchy.h"
#include "config.h"
#include "data_processor.h"
#include "file.h"

using namespace storj;

data_processor::data_processor(const config &cfg) : cfg(cfg)
{}

std::vector<segment> data_processor::split_file(file &f)
{   int reram_reduce = 0;
    //clock_t start,stop;
    //double duration;
    //start=clock();
    // TODO: 改写成流式处理
    // 打开文件
    int fd = open(f.name.c_str(), O_RDONLY);
    if (fd == -1) {
        throw "File not exists";
    }

    // 以 size 为单位读取文件
    char *segment_data = new char[cfg.segment_size];
    int n;
    while ((n = read(fd, segment_data, cfg.segment_size)) > 0) {
        f.segments.emplace_back(std::vector<char>(segment_data, segment_data + n));
    }
    delete[] segment_data;


    // 最后一个 segment 补 0
    if (!f.segments.empty()) {
        segment &s = f.segments.back();
        while (s.data.size() < cfg.segment_size) {
            s.data.emplace_back(0);
        }
    }
    //stop=clock();
    //duration=((double)(stop-start))/CLOCK_TAI;
    //std::cout<<"split_file_time:"<<duration<<std::endl;
    return f.segments;
}

std::vector<stripe> data_processor::split_segment(segment &s) const
{
    //clock_t start,stop;

    //double duration;

   // start=clock();
    // 创建变量，预留空间
    std::vector<stripe> stripes;
    stripes.reserve(s.data.size() / cfg.stripe_size);
    // 以 size 为单位遍历
    for (auto it = s.data.begin(); it < s.data.end(); it += cfg.stripe_size) {
        std::vector<char> stripe_data;
        auto right = it + cfg.stripe_size;
        stripe_data.reserve(right - it);
        stripe_data.insert(stripe_data.end(), std::make_move_iterator(it), std::make_move_iterator(right));
        stripes.emplace_back(stripe_data);
    }
    s.data.clear();
    //stop=clock();

    //duration=((double)(stop-start))/CLOCK_TAI;

   // std::cout<<"split_segment"<<duration<<std::endl;
    return stripes;
}

// std::vector<erasure_share> data_processor::erasure_encode(stripe &s) 
// {
//     // TODO: erasure encode
//     // 创建变量，预留空间
//     std::vector<erasure_share> shares;
//     shares.reserve(cfg.n);
//     // 以 size 为单位遍历
//     for (auto it = s.data.begin(); it < s.data.end(); it += cfg.erasure_share_size) {
//         std::vector<char> erasure_share_data;
//         auto right = it + cfg.erasure_share_size;
//         erasure_share_data.reserve(right - it);
//         erasure_share_data.insert(erasure_share_data.end(), std::make_move_iterator(it), std::make_move_iterator(right));
//         shares.emplace_back(erasure_share_data);
//     }
//     // 未实现 erasure encode，临时填充
//     while (shares.size() < cfg.n) {
//         shares.emplace_back(std::vector<char>(cfg.erasure_share_size, '\0'));
//     }
//     s.data.clear();
//     return shares;
// }

std::vector<erasure_share> data_processor::erasure_encode(stripe &s)
{
   // clock_t start,stop;
//clock_t starte,stope;
   // double duration;

    //start=clock();
    // TODO: erasure encode
    // 创建变量，预留空间
    // 101 char / 10 = 
    // packsize ! 
    int w = 8;
    int packsize = 8;
    int newsize = 0;
    int k = cfg.k;
    int * matrix;
    int * bitmatrix;
    int size = s.data.size();
    std::vector<erasure_share> shares;
    shares.reserve(cfg.n);

    // 计算erasure_share_size的大小并且赋值过去
    /* Find new size by determining next closest multiple */
    newsize = size;
 if (packsize != 0) {
  if (size%(cfg.k*w*packsize*sizeof(long)) != 0) {
   while (newsize%(cfg.k*w*packsize*sizeof(long)) != 0) 
    newsize++;
  }
 }
 else {
  if (size%(cfg.k*w*sizeof(long)) != 0) {
   while (newsize%(cfg.k*w*sizeof(long)) != 0) 
    newsize++;
  }
 }
    int erasure_share_size = newsize / k;
    // std::cout << newsize << " " << s.data.size() << std::endl;
    char * block = new char[newsize];
    std::copy(s.data.begin(), s.data.end(), block);

    // 
    for (int i = size; i < newsize ; i++) {
        block[i] = '\0';
    }

    // std::cout << block << std::endl;
    // 获取生成矩阵大小 w = 8
    matrix = cauchy_original_coding_matrix(cfg.k, cfg.m, w);
 bitmatrix = jerasure_matrix_to_bitmatrix(cfg.k, cfg.m, w, matrix);

 // ! bitmatrix要写入到reram中间,所以计时方式为
 // 512 bit -> 写入
 // m * w * const  const -> 1 ms 
 // cfg.m * cfg.w * 1ms

    // 开始进行encode获取data和coding的数组
    /* Allocate data and coding */
    char ** data = new char*[k];
    char ** coding = new char*[cfg.m];
 for (int i = 0; i < cfg.m; i++) {
        coding[i] = new char[erasure_share_size];
        if (coding[i] == NULL){ perror("malloc"); exit(1); }
 }

    int n = 1;
 int total = 0;
    int extra;
    int readins = 1; // 暂定写死

 while (n <= readins) {
  /* Set pointers to point to file data */
  for (int i = 0; i < k; i++) {
   data[i] = block+(i*erasure_share_size);
  }

  // timing_set(&t3);
  /* Encode according to coding method */
//   std::cout << erasure_share_size << std::endl;
   // clock_t starte,stope;

   // double duratione;

   // starte=clock();
    
    // ！这里计算encode的时间
    jerasure_bitmatrix_encode(cfg.k, cfg.m, w, bitmatrix, data, coding, erasure_share_size, packsize);
    // ！这里计算encode的时间

    // reram -> encode (x) ｜ (erasure_share_size * 8 / 512) 最小等于1 * const 1ms

    // !每次需要读的数据为
    // (erasure_share_size * 8 / 512) 最小等于1 * const 1ms * cfg.k
    //
    
   // stope=clock();

    //duratione=((double)(stope-starte))/CLOCK_TAI;

   // std::cout<<"Encode"<<duratione<<std::endl;
  for (int i = 1; i <= k; i++) {
      if (i == k) {
         // std::cout << data[i-1] << std::endl;
      }
      std::vector<char> stripe_data(data[i-1], data[i-1] + erasure_share_size);
   shares.emplace_back(stripe_data);
            // bzero(data[i-1], cfg.erasure_share_size));
  }
  for (int i = 1; i <= cfg.m; i++) {
      std::vector<char> stripe_data(coding[i-1], coding[i-1] + erasure_share_size);
            shares.emplace_back(stripe_data);
            // bzero(data[i-1], cfg.erasure_share_size));
  }
  n++;
 }
    free(block);
    free(matrix);
    free(bitmatrix);
    free(coding);
    s.data.clear();
    // stop=clock();

   // duration=((double)(stop-start))/CLOCK_TAI;

    //std::cout<<"erasure_encode:"<<duration<<std::endl;
    return shares;
}

std::vector<piece> data_processor::merge_to_pieces(std::vector<std::vector<erasure_share>> &s) const
{
    // 粒度为 stripe -> erasure share
    // s[i][j] 是单个 stripe 切分出来的单个 erasure share
    // s[i]    是单个 stripe 切分出来的所有 erasure shares
    // 即，需要交换遍历维度以转换成 piece
    //clock_t start,stop;

   // double duration;

   // start=clock();
    std::vector<piece> pieces;
    pieces.reserve(cfg.n);
    for (int y = 0; y < cfg.n; y++) {
        piece p;
        p.erasure_shares.reserve(s.size());
        p.data.reserve(s.size() * cfg.erasure_share_size);
        for (auto &shares: s) {
            erasure_share &share = shares[y];
            p.data.insert(p.data.end(), std::make_move_iterator(share.data.begin()), std::make_move_iterator(share.data.end()));
            p.erasure_shares.emplace_back(share);
        }
        pieces.emplace_back(p);
    }
    // stop=clock();

    //duration=((double)(stop-start))/CLOCK_TAI;

    //std::cout<<"merge_to_pieces"<<duration<<std::endl;
    return pieces;
}

std::vector<erasure_share> data_processor::split_piece(piece &p) const
{
    //clock_t start,stop;

    //double duration;

   // start=clock();
    // 创建变量，预留空间
    std::vector<erasure_share> shares;
    shares.reserve(p.data.size() / cfg.erasure_share_size);
    // 以 size 为单位遍历
    for (auto it = p.data.begin(); it < p.data.end(); it += cfg.erasure_share_size) {
        std::vector<char> erasure_share_data;
        auto right = it + cfg.erasure_share_size;
        erasure_share_data.reserve(right - it);
        erasure_share_data.insert(erasure_share_data.end(), std::make_move_iterator(it), std::make_move_iterator(right));
        shares.emplace_back(erasure_share_data);
    }

    for(int i = 0; i< (cfg.n - shares.size()) ; i++) {
        erasure_share temp;
        shares.push_back(temp);
    }

    p.data.clear();
     //stop=clock();

    //duration=((double)(stop-start))/CLOCK_TAI;

    //std::cout<<"split_piece"<<duration<<std::endl;
    return shares;
}

// std::vector<stripe> data_processor::merge_to_stripes(std::vector<std::vector<erasure_share>> &s) const
// {
//     // 粒度为 piece -> erasure share
//     // s[i][j] 是单个 piece 切分出来的单个 erasure share
//     // s[i]    是单个 piece 切分出来的所有 erasure shares
//     // 即，需要交换遍历维度以转换成 stripe
//     std::cout << "test !!!!" << std::endl;
//     std::vector<stripe> stripes;
//     // 未实现 erasure decode，临时处理数据，只需前 k 个 piece
//     for (int x = 0; x < cfg.segment_size / cfg.stripe_size; x++) {
//         stripe stripe;
//         stripe.data.reserve(cfg.stripe_size);
//         for (int y = 0; y < cfg.k; y++) {
//             erasure_share &share = s[y][x];
//             for(int i = 0; i < share.data.size() ;i ++) {
//                 if(share.data[i] != '\0') {
//                     stripe.data.push_back(share.data[i]);
//                 }
//             }
//             // stripe.data.insert(stripe.data.end(), std::make_move_iterator(share.data.begin()), std::make_move_iterator(share.data.end()));
//         }
//         stripes.emplace_back(stripe);
//     }
//     // TODO: erasure decode
//     return stripes;
// }

std::vector<stripe> data_processor::merge_to_stripes(std::vector<std::vector<erasure_share>> &s) const
{
    // s[n]的某一个元素的size为0,则说明了丢失了

//clock_t start,stop;
//clock_t startd,stopd;
   // double duration;

    //start=clock();
    std::vector<stripe> stripes;
    int numerased = 0;
    char **data;
	char **coding;
	int *erasures;
	int *erased;
	int *matrix;
	int *bitmatrix;
    int w = 8;
    int packetsize = 8;
    int k = cfg.k;
    int m = cfg.m;

    matrix = cauchy_original_coding_matrix(k, m, w);
    bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, matrix);

    // !

    erased = (int *)malloc(sizeof(int)*(cfg.k+cfg.m));
	for (int i = 0; i < k+m; i++)
		erased[i] = 0;
	erasures = (int *)malloc(sizeof(int)*(cfg.k+cfg.m));
    for (int i = 0; i < k+m; i++)
		erasures[i] = -2;

	data = (char **)malloc(sizeof(char *)*k);
	coding = (char **)malloc(sizeof(char *)*m);
	
    int blocksize = 0;

    for (int x = 0; x < cfg.segment_size / cfg.stripe_size; x++) {
        // 说明了有cfg.segment_size / cfg.stripe_size 个stripe
        for (int y = 0; y < cfg.k + cfg.m; y++) {
            
            // 读所有的切分的内容啊
            erasure_share &share = s[y][x];

            // 当前的erasure_share
            // if (share.data.size() != 512) {
            //     std::cout << share.data.size()<<std::endl;
            // }
            
            if (share.data.size() == 0 ) {
                //std::cout << "!!!! win !!!! 日你妈" << std::endl;
				erased[y] = 1;
				erasures[numerased] = y;
				numerased++;
				//printf("%s failed\n", fname);
			}
			else {
                // std::cout << share.data.size() << std::endl;
                // data[y] = (char *)malloc(sizeof(char)*share.data.size());
                blocksize = share.data.size();
                
                if(y < cfg.k ) {
                    data[y] = (char *)malloc(sizeof(char)*share.data.size());
                    std::copy(share.data.begin(), share.data.end(), data[y]);
                }else {
                    coding[y - k] = (char *)malloc(sizeof(char)*share.data.size());
                    std::copy(share.data.begin(), share.data.end(), coding[y - k]);
                }
                
			}

        }
        /* Finish allocating data/coding if needed */
        for (int i = 0; i < numerased; i++) {
            if (erasures[i] < k) {
                data[erasures[i]] = (char *)malloc(sizeof(char)*blocksize);
            }
            else {
                coding[erasures[i]-k] = (char *)malloc(sizeof(char)*blocksize);
            }
        }
		erasures[numerased] = -1;

       // clock_t startd,stopd;

    //double durationd;

   //startd=clock();
        // ! decode | 
        jerasure_bitmatrix_decode(k, m, w, bitmatrix, 0, erasures, data, coding, blocksize, packetsize);
        stripe stripe_inner;
   // stopd=clock();

    //durationd=((double)(stopd-startd))/CLOCK_TAI;

    //std::cout<<"Decode"<<durationd<<std::endl;
        // std::cout << data[0] << std::endl;
        // std::cout << s[0][x].data[0] << s[0][x].data[1];
        for (int i = 0; i< cfg.k; i++) {
            if(i == (cfg.k - 1)) {
                    // std::cout << data[i] << std::endl;
                }
            for(int j =0 ;j < blocksize; j++) {
                if(data[i][j] != '\0')
                    stripe_inner.data.push_back(data[i][j]);
            }
        }
        numerased = 0;
        stripes.emplace_back(stripe_inner);
        // data.insert(stripe.data.end(), std::make_move_iterator(share.data.begin()), std::make_move_iterator(share.data.end()));
    }
    free(data);
    free(coding);
    free(erasures);
	free(erased);
    // stop=clock();

    //duration=((double)(stop-start))/CLOCK_TAI;

   // std::cout<<"merge_to_stripes"<<duration<<std::endl;
    return stripes;
}


segment data_processor::merge_to_segment(std::vector<stripe> &stripes) const
{
    segment res;
    for (auto &stripe: stripes) {
        res.data.insert(res.data.end(), std::make_move_iterator(stripe.data.begin()), std::make_move_iterator(stripe.data.end()));
    }
    return res;
}

file data_processor::merge_to_file(std::vector<segment> &segments) const
{
    //clock_t start,stop;

    //double duration;

    //start=clock();
    file res;
    res.segments = segments;
    // stop=clock();

    //duration=((double)(stop-start))/CLOCK_TAI;

    //std::cout<<"merge_to_file:"<<duration<<std::endl;
    return res;
}

std::vector<stripe> data_processor::repair_stripes_from_erasure_shares(const std::vector<std::vector<erasure_share>> &s) const
{
    // TODO: implementation
}


