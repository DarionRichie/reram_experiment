import sqlite3

con = sqlite3.connect('storj.db')

cur = con.cursor()

# remove table

try:
    ret = cur.execute('drop table file')
    print('drop succ! file')
except :
    print('no table file')


try:
    ret = cur.execute('drop table piece')
    print('drop succ! piece')
except :
    print('no table piece')

try:
    ret = cur.execute('drop table segment')
    print('drop succ! -- segment')
except :
    print('no table segment')

try:
    ret = cur.execute('drop table storage_node')
    print('drop succ! -- storage_nodes')
except :
    print('no table -- storage_nodes')



