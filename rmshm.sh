#!/bin/bash

# 获取共享内存信息
ipcs_output=$(ipcs -m)

# 提取共享内存标识符（shmid）
shmid_list=$(echo "$ipcs_output" | awk 'NR>3 {print $2}')

echo $shmid_list

# 循环遍历每个 shmid 并删除
for shmid in $shmid_list; do
    echo "Deleting shared memory segment with shmid: $shmid"
    ipcrm -m $shmid
done


