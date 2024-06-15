# 打开文件
f=open('D1056.sh','w')

start_time = 1.5
# 保存要求的数据库
nodes = 1056
all_time = 0.02
state = ["static_adapt", "reconfig_adapt"] # "reconfig_ecmp", "reconfig_adapt", "reconfig_min"
reconfig_time = [0.0002, 0.0001] # 0.0002, 0.0001
traffic_pattern = ["adverse", "neighbor"] # "adverse", "neighbor"
app_bd = ["10Gbps"] # "5Gbps", "10Gbps"
congestionMonitorPeriod = ["0.0001","0.001", "0.01"]
ocs = [33]
bias = [100, 1000, 10000]
reconfig_count = [50, 100, 150] # 50, 100, 200
maxBtyes = [30000000]


cmd1 = "mkdir "
cmd2 = "cp ./parse-results.py ./"
cmd3 = "cp ./rn-simulator-test ./"
cmd41 = "(cd ./"
cmd42 = " && nohup ./rn-simulator-test "
cmd43 = ""
cmd44 = " > output.txt 2>&1 &)"

# for
dir_name = ""
check_cmd43 = []
for pattern in traffic_pattern:
    for state_i in state:
        for retime in reconfig_time:
            for bd in app_bd:
                for congestionMonitorPeriod_i in congestionMonitorPeriod:
                    for ocs_i in ocs:
                        for bias_i in bias:
                            for reconfig_count_i in reconfig_count:
                                for maxBtyes_i in maxBtyes:
                                    cmd43 += " -max_bytes=" + str(maxBtyes_i) + " "
                                    cmd43 = " -stop_time=" + str(all_time + start_time) + " "
                                    if(nodes == 1056):
                                        cmd43 += " -p=4 -a=8 -h=4 "
                                    elif(nodes == 2550):
                                        cmd43 += " -p=5 -a=10 -h=5 "
                                    elif(nodes == 5256):
                                        cmd43 += " -p=6 -a=12 -h=6 "
                                    dir_name = pattern + "_" + str(nodes) + "_" + str(all_time) + "s_" + state_i + "_" + str(retime) + "reconfig_" + bd + "_" + congestionMonitorPeriod_i + "cong_" + str(ocs_i) + "l_" + str(bias_i) + "bias_" + str(reconfig_count_i) + "reconfig_count_" + str(maxBtyes_i) + "maxBtyes"
                                    if(pattern == "adverse"):
                                        cmd43 += " --is_ad "
                                    if("ecmp" in state_i):
                                        cmd43 += " --ecmp "
                                    if("adapt" in state_i):
                                        cmd43 += " --ugal "
                                    if("noreconfig" not in state_i and "reconfig" in state_i):
                                        cmd43 += " --enable_reconfig "
                                        cmd43 += " -reconfig_time=" + str(retime) + " "
                                        cmd43 += " -reconfiguration_count=" + str(reconfig_count_i) + " "
                                    cmd43 += " -app_bd=" + bd + " "
                                    if("adapt" in state_i):
                                        cmd43 += " -congestion-monitor-period=" + congestionMonitorPeriod_i + " "
                                        cmd43 += "-bias=" + str(bias_i) + " "
                                    if("reconfig" in state_i):
                                        cmd43 += " -ocs=" + str(ocs_i) + " "
                                    if(cmd43 in check_cmd43):
                                        continue
                                    else:
                                        check_cmd43.append(cmd43)
                                    f.write(cmd1 + dir_name + "\n")
                                    f.write(cmd2 + dir_name + "\n")
                                    f.write(cmd3 + dir_name + "\n")
                                    f.write(cmd41 + dir_name + cmd42 + cmd43 + cmd44 + "\n")
                                    f.write("\n")


f.close()