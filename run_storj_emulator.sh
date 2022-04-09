# 删除datatest文件
./remove_data.sh
# drop 所有table
python remove_table.py
# run main程序
./storj_emulator $1 $2 $3 $4 $5 $6
