#include <fcntl.h>
#include <iostream>

#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <fstream>
#include "storj/data_manager.h"

#include <vector>
#include <sstream>

#include "storj/data_processor.h"

const std::string &FILENAME_IN = "datatest_2.txt";
const std::string &FILENAME_OUT = "datatest_3.txt";

storj::data_manager *manager = new storj::data_manager();
bool running = true;

// int Find_erasure_size()
// {
//     std::string file = "./test_data.txt";
//     std::ifstream ifs(file.c_str(), std::fstream::in);

//     if(!ifs)
//         std::cout << "Read file failed!" << std::endl;
//     //obtain size of file in Byte
//     ifs.seekg(0, ifs.end);
//     int length = ifs.tellg();
//     std::cout << "length: " << length << std::endl;

//     std::string line;
//     //check '\n' from second character because the last character is '\n'
//     int index = -2;
//     int linenum = 0;
//     while(length)
//     {
//         char c;
//         ifs.seekg (index, ifs.end);
//         ifs.get(c);
//         //check '\n' from end to begin
//         if(c == '\n')
//         {
//             //get the the last line when finding its corresponding beginning
//             std::getline(ifs, line);
//             //convet characters to int through istringstream class provided in c++
//             std::istringstream iss(line);
//             iss >> linenum;
//             break;
//         }
//         length--;
//         index--;
//     }
//     std::cout << "line num: " << linenum << std::endl;
//     return linenum;
// }

void thread_scanner_func()
{
    // int reram_reduce = 0;
    while (running)
    {
        std::cout << "new loop !\n";
        // 扫描出需要修复的 segments
        std::tuple<std::vector<std::string>, std::vector<int>, std::vector<int>, std::unordered_map<std::string, int>> tuple = manager->scan_corrupted_segments();
        std::vector<std::string> &segment_ids = std::get<0>(tuple);
        std::vector<int> &ks = std::get<1>(tuple);
        std::vector<int> &rs = std::get<2>(tuple);
        std::unordered_map<std::string, int> &corrupted_segmetn_size = std::get<3>(tuple);
        // segments 排序
        storj::data_manager::sort_segments(segment_ids, ks, rs);
        //修复 segments
        std::cout << "scan the segment size is : " << segment_ids.size() << std::endl;
        if (segment_ids.size() == 0)
        {
            break;
        }
        // for (const auto &segment_id: segment_ids) {
        //     manager->repair_segment(segment_id);
        //  }
        //打印需要的semgents ->
        for (const auto &one_segment : segment_ids)
        {
            std::cout << "this segment must be repair : " << one_segment << std::endl;
            //  clock_t start,stop;
            // double duration;
            // start=clock();

            std::ofstream mycout("test_data.txt", std::ios::app);
            auto iter = corrupted_segmetn_size.begin();
            while (iter != corrupted_segmetn_size.end())
            {
                mycout << "file & corrupted size" << iter->first << "," << iter->second << std::endl;
                ++iter;
            }

            manager->repair_segment(one_segment);
            //    stop=clock();
            //    duration=((double)(stop-start))/CLOCK_TAI;
            //    std::cout<<"!time:"<<duration<<std::endl;

            // std::ofstream mycout("test_data.txt",std::ios::app);
            // int erasure_size=Find_erasure_size()
            // //int reram_time = std::min(erasure_size*8/512,1);
            // mycout<<"Jerasure Repair time:"<<duration<<std::endl;
            // mycout<<"Reram Repair time:"<<duration+reram_time-reram_reduce<<std::endl;
            // mycout<<"Reram Repair time:"<<duration+reram_time<<std::endl;
            // mycout.close();
        }
        // 等待 30s
        sleep(30);
        // std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

int main()
{
    // std::thread t2(thread_scanner_func);
    // t2.join();
    thread_scanner_func();
    return 0;
}
