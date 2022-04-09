# 上传文件
./run_storj_emulator.sh $1 $2 $3 $4 $5 $6
# 扫描
./run_storj_scan.sh

 python read_log.py
