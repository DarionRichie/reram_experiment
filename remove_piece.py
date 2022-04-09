import sqlite3
import os

con = sqlite3.connect("storj.db")

cur = con.cursor()
num = 1
num2 = 1
while(num2>0):
       cur.execute('select storage_node_id from piece ORDER BY RANDOM() limit ?',(num,) )
       for x in cur:
           temp = x[0]
       #print(k)
       
       del_dir = "./storage_nodes/"+temp
      #print(del_dir)
       del_list = os.listdir(del_dir)
       #print(del_list)
       for file in del_list:
              filePath = os.path.join(del_dir, file)
              print(filePath)
              # print(os.path.join(del_dirs, file)
              os.remove(filePath)
       
       num2 -= 1
cur.close()
