# reram_experiment
主体文件在 : storj/目录下

main.cpp // reram 上传文件 + 下载文件 + 对比<br>
test_main.cpp // reram 定时扫描缺失的segment<br>
 
run_storj_scan.sh // 运行test_main的二进制<br>
run_storj_emulator.sh // 运行main.cpp 二进制<br>

remove_data.sh // 删除测试文件txt + storage_nodes目录<br>
remove_piece.sh // 删除文件目录的piece文件<br>

wait.sh // 之前定制化跑数脚本<br>
wait2.sh // 一样(后期跑数只需要修改这样的脚本)<br>

read_log.py // 读取cpp写入日志的文件,输出到test_data.csv中<br>

total_run.sh // reram 上传文件 + reram定时扫描 + test_data.csv日志规范化<br>

storj目录下请看：<br>
data_processor.cpp <br>

erasure_encode 函数 接入了纠删码部分<br>
merge_to_stripes 函数 接入了纠删码部分

