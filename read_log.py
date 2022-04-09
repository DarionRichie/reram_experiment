# coding = utf-8
import time
config_dict = {
    'file_size' : 1

}


total_txt = ''
with open('/home/qwe/Desktop/Re_storj/storjtest/Storj_local/test_data.txt', 'r') as f:
    total_txt =  f.read()
    f.close()



value_array = []
value_header = []

print("parse begin !!!1")
total_line = total_txt.strip().split('\n') 
for index in range(len(total_line)):
    # print(total_line[index].split(":"))
    # now_line = total_line[index].split(":")
    if index == 2:
         now_line = total_line[index].split(",")
         print(now_line)
         for item in now_line:
             now_split = item.split(":")
             print(now_split[0].strip(), now_split[1].strip())
             value_array.append(now_split[1].strip())
             value_header.append(now_split[0].strip())

    if index == 3 or index == 5 or index == 7  or index == 10 or index == 11:
        now_line = total_line[index].split(":")
        print(now_line[0].strip(), now_line[1].strip() + ' ns')
        
        value_array.append(now_line[1].strip())
        value_header.append(now_line[0].strip())
    
    if index == 4 or index == 6 or index == 8 or index == 9 or index == 12:
        now_line = total_line[index].split(":")
        print(now_line[0].strip(), now_line[1].strip())

        value_array.append(now_line[1].strip())
        value_header.append(now_line[0].strip())
print(value_array)
print(value_header)

# add the value_arry to csv

with open('./test_data.csv', 'aw') as f:
    f.write(','.join(value_array) + '\n')
    f.close()
    # two delete it ! unuse

    # file_size , segment_size, stripe_size , k, m, n

    # piece to erasure time

    # Stripe Decode time

    # stripe 
