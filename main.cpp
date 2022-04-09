#include <fcntl.h>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <unistd.h>

#include "storj/data_manager.h"

const std::string &FILENAME_IN = "datatest_2.txt";
const std::string &FILENAME_OUT = "datatest_3.txt";

storj::data_manager *manager;
bool running = false;

void thread_scanner_func()
{
    while (running)
    {
        // 扫描出需要修复的 segments
        std::tuple<std::vector<std::string>, std::vector<int>, std::vector<int>, std::unordered_map<std::string, int>> tuple = manager->scan_corrupted_segments();
        std::vector<std::string> &segment_ids = std::get<0>(tuple);
        std::vector<int> &ks = std::get<1>(tuple);
        std::vector<int> &rs = std::get<2>(tuple);
        // segments 排序
        storj::data_manager::sort_segments(segment_ids, ks, rs);
        // 修复 segments
        for (const auto &segment_id : segment_ids)
        {
            manager->repair_segment(segment_id);
        }

        // 等待 30s
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

bool compare_file(const std::string &filename1, const std::string &filename2)
{
    int fd1 = open(filename1.c_str(), O_RDONLY);
    int fd2 = open(filename2.c_str(), O_RDONLY);
    if (fd1 == -1 || fd2 == -1)
    {
        perror("Failed to open file");
        return false;
    }
    const int unit = 10 << 2;
    char buf1[unit] = {0};
    char buf2[unit] = {0};
    int n1;
    int n2;
    while (true)
    {
        n1 = read(fd1, buf1, unit);
        n2 = read(fd2, buf2, unit);
        if (n1 != n2)
        {
            close(fd1);
            close(fd2);
            puts("111");
            return false;
        }
        if (n1 <= 0)
        {
            break;
        }
        for (int i = 0; i < unit; i++)
        {
            if (buf1[i] != buf2[i])
            {
                close(fd1);
                close(fd2);
                puts("222");
                return false;
            }
        }
    }
    close(fd1);
    close(fd2);
    return true;
}

void create_file(int size)
{
    const int unit = 10 << 2;
    // Write the file
    int fd = open(FILENAME_IN.c_str(), O_CREAT | O_RDWR);
    if (fd == -1)
    {
        perror("Failed to open file");
        return;
    }
    char buf[unit];
    for (int i = 0; i < unit; i++)
    {
        buf[i] = i % 10 + '0';
    }
    for (int i = 0; i < size; i += unit)
    {
        write(fd, buf, std::min(unit, size - i));
    }
    close(fd);
}

int main(int argc, char **argv)
{
    manager = new storj::data_manager();

    storj::config cfg;
    // cfg.file_size = 5* 1024 * 1024 ;
    // cfg.segment_size = 1 * 1024 * 1024;
    // cfg.stripe_size = 1024 * 1024;

    if (argc != 7)
    {
        std::cout << "check argv !!!! -- usage -- exec file_size segment_size stripe_size k m n" << std::endl;
        exit(0);
    }

    std::cout << std::atoi(argv[1]) << std::endl;
    cfg.file_size = std::atoi(argv[1]);
    cfg.segment_size = std::atoi(argv[2]);
    cfg.stripe_size = std::atoi(argv[3]);
    std::cout << "file_size : " << cfg.file_size << " segment_size : " << cfg.segment_size << " stripe: size " << cfg.stripe_size << std::endl;

    // cfg.erasure_share_size = 512;

    cfg.k = std::atoi(argv[4]);
    cfg.m = std::atoi(argv[5]);
    cfg.n = std::atoi(argv[6]);

    std::cout << cfg.k << " is k  " << cfg.m << " is m  " << cfg.n << " is n" << std::endl;
    create_file(cfg.file_size);
    puts("file created");

    manager->upload_file(FILENAME_IN, cfg);
    std::cout << "upload succ!" << std::endl;
    const storj::file &file = manager->download_file(FILENAME_IN);
    // std::cout << "main func \n";
    // for (const auto c : file.segments[0].data) std::cout << c << " ";
    // std::cout << std::endl;
    // std::cout << file.segments[0].data.size() << std::endl;
    // 下载到的文件仍在内存中，写到硬盘
    {
        // 删除硬盘中的 data-out.txt
        remove(FILENAME_OUT.c_str());

        // 写文件
        const int unit = 10 << 2;
        int fd = open(FILENAME_OUT.c_str(), O_CREAT | O_RDWR);
        if (fd == -1)
        {
            perror("Failed to open file");
            return 1;
        }
        for (int i = 0; i < file.segments.size(); i++)
        {
            const storj::segment &segment = file.segments[i];

            // std::cout << i << std::endl;

            for (int j = 0; j < segment.data.size(); j += unit)
            {
                int size = std::min(unit, (int)segment.data.size() - j);

                // 写到最后一个 segment 时，可能有冗余数据
                // if (i == file.segments.size() - 1) {
                //    size = std::min(size, cfg.file_size % cfg.segment_size - j);
                //}
                write(fd, segment.data.data() + j, size);
            }
        }
        close(fd);
        puts("downloaded");
    }

    // 比较上传与下载的文件内容是否相同
    if (compare_file(FILENAME_IN, FILENAME_OUT))
    {
        std::cout << "Same" << std::endl;
    }
    else
    {
        std::cout << "!!! DIFFERENT !!!" << std::endl;
    }

    // running = true;
    // std::thread thread_scanner(thread_scanner_func);
    // thread_scanner.join();
    delete manager;
    return 0;
}
