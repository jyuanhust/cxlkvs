#!/bin/bash

# 多次执行某可执行文件

exec_file=$1       # 第一个参数：要执行的文件路径
count=$2           # 第二个参数：运行次数
shift 2            # 移除前两个参数，剩下的当作可执行文件的参数

for ((i = 1; i <= count; i++)); do
    echo "第 $i 次执行: $exec_file $@"
    "$exec_file" "$@"   # 执行并传递剩余参数
    echo "-----------------------------"
done

