#!/usr/bin/python3

import os

## 原始数据集生成
ycsb_dir = "~/Projects/ycsb-0.17.0/"

workload_spec_dir = 'workload_spec2/'

workload_dir = "workloads/"

output_dir = "workloads/"


for file in os.listdir(workload_spec_dir):
    workload_spec = os.path.join(workload_spec_dir, file)
    out_ycsb_load = os.path.join(output_dir, 'ycsb_load_' + file)
    out_ycsb_txn = os.path.join(output_dir, 'ycsb_txn_' + file)
    if os.path.isfile(workload_spec):
        print(workload_spec)
        cmd_ycsb_load = ycsb_dir + 'bin/ycsb load basic -P ' + workload_spec + ' -s > ' + out_ycsb_load
        cmd_ycsb_txn = ycsb_dir + 'bin/ycsb run basic -P ' + workload_spec + ' -s > ' + out_ycsb_txn
        
        # print(cmd_ycsb_load)
        # print(cmd_ycsb_txn)
        os.system(cmd_ycsb_load)
        os.system(cmd_ycsb_txn)

## 整理数据集

workload_dir = "workloads/"

output_dir = "workloads_arrange/"

for file in os.listdir(workload_dir):
    print(file)
    if "_load_" in file:
        # 整理load文件，均为insert
        f_load = open(os.path.join(workload_dir, file), 'r')
        f_load_out = open(os.path.join(output_dir, file), 'w')
        for line in f_load :
            cols = line.split()
            if len(cols) > 0 and cols[0] == "INSERT":
                f_load_out.write (cols[0] + " " + cols[2][4:] + "\n")
        f_load.close()
        f_load_out.close()
        
    elif "_txn_" in file:
        # 整理run文件
        f_txn = open(os.path.join(workload_dir, file), 'r')
        f_txn_out = open(os.path.join(output_dir, file), 'w')
        for line in f_txn :
            cols = line.split()
            if (cols[0] == 'SCAN') or (cols[0] == 'INSERT') or (cols[0] == 'READ') or (cols[0] == 'UPDATE'):
                startkey = cols[2][4:]
                if cols[0] == 'SCAN' :
                    numkeys = cols[3]
                    f_txn_out.write (cols[0] + ' ' + startkey + ' ' + numkeys + '\n')
                else :
                    f_txn_out.write (cols[0] + ' ' + startkey + '\n')
        f_txn.close()
        f_txn_out.close()
        