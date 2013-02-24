#!/usr/bin/python
import sys
import serial
import time
import argparse

# Reads in a font dictionary from a file and returns it in the form of a 9 by n array of 1s and 0s
def parseFontDict(filename):
    f = open(filename)
    fontdict = dict()
    while True:
        c = f.readline()
        if c=='': break
        bitarray = []
        for i in range(9):
            row_str = f.readline()
            row = map(int,row_str.split())
            bitarray.append(row)
        fontdict[c[0]]=bitarray

    return fontdict

def makebitarray(words,fontdict):
    raw_array = []
    for i in range(len(words)):
        raw_array+=map((lambda x:fontdict[x]),words[i])
        raw_array+=[fontdict[' ']]
    return raw_array

font = parseFontDict('fonts')
#do some argument parsing
parser = argparse.ArgumentParser()
parser.add_argument('-b','--brightness',type=int,default=6,choices=range(8))
parser.add_argument('-s','--speed',type=int,default=4,choices=range(8))
parser.add_argument('-i','--msg_ind',type=int,required=True,choices=range(4))
parser.add_argument('msg',nargs='*')
params = parser.parse_args()
#compute the initial byte of serial transmission
msg_params = params.msg_ind*64+params.brightness*8+params.speed
#construct the raw bit array with nothing in between
raw_bit_array = makebitarray(params.msg,font) #map((lambda x:font[x]),sys.argv[1])
print 
#construct the spaced bit array
spaced_bit_array = []
for i in range(9):
    spaced_bit_array.append([])

for i in range(len(raw_bit_array)):
    for j in range(9):
        if i==0:
            spaced_bit_array[j] = raw_bit_array[i][j]
        else:
            spaced_bit_array[j] = spaced_bit_array[j]+[0]+raw_bit_array[i][j]

#construct the big bytearray
arr = bytearray()
for i in range(9):
    spaced_bit_array[i]+=[0]*(8-len(spaced_bit_array[i])%8)
    for j in range(0,len(spaced_bit_array[i]),8):
        arr.append(reduce((lambda x,y:2*x+y),spaced_bit_array[i][j:j+8]))
#make a 2-byte 'integer' to send to the arduino containing parameters
n_cols_array = bytearray()
n_cols_array.append(msg_params)
n_cols_array.append(len(spaced_bit_array[0])/8)

for i in arr: 
    for j in [7,6,5,4,3,2,1,0]:
       print i/(2**j),
       i = i%(2**j)
    print

#open the serial connection
comm = object()
for i in [0,1,2,3,4,5]:
    try:
        comm = serial.Serial('/dev/ttyACM'+str(i),baudrate=19200,dsrdtr=False)
        break
    except serial.SerialException:
        print 'Nope'
        continue
#Hack to prevent arduino from resetting
comm.setDTR(level=False)
time.sleep(2)

comm.write(n_cols_array)
comm.write(arr[:50])
print len(arr)
for i in range(50,len(arr),48):
    time.sleep(0.4)
    print comm.write(arr[i:i+48])
    print i

comm.close()
