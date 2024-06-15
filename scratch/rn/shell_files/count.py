# should be put in ./build
# use trace system(.pcap files) to get packet trace on every node
# to compute drop rate and average delay 
# need to change l if the (sender,receiver) pairs are changed
import os

send = 0
receive_send = 0
ack = 0
receive_ack = 0

for i in range(0, 72):
    s = "tcpdump -nn -tt -r ./bottleneckTcp-" + str(i) + "-0.pcap > " + str(i) + ".txt"
    os.system(s)

# sender v.s. receiver
l = [[0,55],
[70,9],
[43,41],
[7,10],
[12,33],
[37,50],
[67,59],
[22,60],
[18,52],
[64,35],
[8,15],
[69,62],
[26,19],
[54,66],
[5,39],
[1,16],
[34,45],
[47,32],
[49,14],
[20,11],
[29,46],
[44,4],
[68,56],
[17,57],
[65,36],
[63,38],
[2,51],
[24,48],
[28,30],
[27,61],
[40,13],
[31,42],
[58,53],
[21,25],
[3,6],
[71,23]]

for i in range(len(l)):
    # receiver
    s_ = str(l[i][0]) + ".txt"
    f = open(s_, "r")
    
    s_ = "1.1." + str(l[i][1] + 1) + ".2.49153 > 1.1." + str(l[i][0] + 1) + ".2.50000"
    for line in f:
        # s = line.split()
        if (s_ in line):
            receive_send += 1
    f.close()
    
    s_ = str(l[i][0]) + ".txt"
    f = open(s_, "r")
    s_ = "1.1." + str(l[i][0] + 1) + ".2.50000 > 1.1." + str(l[i][1] + 1) + ".2.49153"
    for line in f:
        # s = line.split()
        if (s_ in line):
            ack += 1

    f.close()

    # sender
    s_ = str(l[i][1]) + ".txt"
    f = open(s_, "r")

    s_ = "1.1." + str(l[i][1] + 1) + ".2.49153 > 1.1." + str(l[i][0] + 1) + ".2.50000"
    for line in f:
        # s = line.split()
        if (s_ in line):
            send += 1
    f.close()

    s_ = str(l[i][0]) + ".txt"
    f = open(s_, "r")
    s_ = "1.1." + str(l[i][0] + 1) + ".2.50000 > 1.1." + str(l[i][1] + 1) + ".2.49153"
    for line in f:
        # s = line.split()
        if (s_ in line):
            receive_ack += 1

    f.close()

print(send + ack, receive_ack + receive_send, send + ack - receive_ack - receive_send)
print(send, receive_send, send - receive_send)

count = 0.0
time = 0.0
for i in range(len(l)):
    d = {}
        # sender
    s_ = str(l[i][1]) + ".txt"
    f = open(s_, "r")

    s_ = "1.1." + str(l[i][1] + 1) + ".2.49153 > 1.1." + str(l[i][0] + 1) + ".2.50000"
    for line in f:
        s = line.split()
        if (s_ in line):
            d[s[8]] = s[0]
    f.close()

    # receiver
    s_ = str(l[i][0]) + ".txt"
    f = open(s_, "r")
    
    s_ = "1.1." + str(l[i][1] + 1) + ".2.49153 > 1.1." + str(l[i][0] + 1) + ".2.50000"
    for line in f:
        s = line.split()
        if (s_ in line and s[8] in d.keys()):
            count += 1
            time += float(s[0]) - float(d[s[8]])
    f.close()

print(time / count)

