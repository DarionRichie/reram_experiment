# 删除piece
echo '' > nohup.out
echo '' > test_data.txt
python remove_piece.py
# run scan
nohup ./storj_emulator_scan 
