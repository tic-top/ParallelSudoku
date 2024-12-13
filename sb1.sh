#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)
# Set up batch job settings

#SBATCH --job-name=sudoku
#SBATCH --mail-type=BEGIN,END
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem-per-cpu=1g
#SBATCH --time=00:05:00
#SBATCH --account=cse587f24_class
#SBATCH --partition=standard

# Load necessary modules
module load gcc
module load openmpi

# Compile the serial code
g++ -O3 serial.cpp -o serial

input_csv="sudoku.csv"
output_csv="output1.csv"

# 输出CSV表头
echo "puzzle,solution,clues,difficulty,difficulty_range,result,runtime" > "$output_csv"

# 初始化行计数
line_count=0

# 启动 serial 程序并保持其运行
# 启动 serial 程序并让它处于等待状态，接收标准输入
./serial &

# 获取 serial 程序的进程ID
serial_pid=$!

# 统计总运行时间
st=$(date +%s)
tail -n +2 "$input_csv" | while IFS=, read -r puzzle solution clues difficulty difficulty_range; do
    line_count=$((line_count + 1))
    puzzle=$(echo "$puzzle" | sed 's/\r//g' | tr -d '\n')
    solution=$(echo "$solution" | sed 's/\r//g' | tr -d '\n')
    clues=$(echo "$clues" | sed 's/\r//g' | tr -d '\n')
    difficulty=$(echo "$difficulty" | sed 's/\r//g' | tr -d '\n')
    difficulty_range=$(echo "$difficulty_range" | sed 's/\r//g' | tr -d '\n')

    # 启动程序并传递数独题目
    start_time=$(date +%s%3N)

    # 将数独通过管道传递给程序
    result_and_time=$(echo "$puzzle" | tee /dev/tty | ./serial)
    
    # 计算运行时间
    end_time=$(date +%s%3N)
    elapsed_time=$((end_time - start_time))

    # 假设输出格式是：解答 runtime
    result=$(echo "$result_and_time" | awk '{print $1}')
    runtime=$(echo "$result_and_time" | awk '{print $2}')
    echo "Runtime: $runtime, Esttime: ${elapsed_time}ms"

    # 将结果写入CSV文件
    echo "$puzzle,$solution,$clues,$difficulty,$difficulty_range,$result,$runtime" >> "$output_csv"
done

# 向 serial 程序发送 "exit" 命令来退出
echo "exit" > /dev/tty

# 等待 serial 程序结束
wait $serial_pid

end_time=$(date +%s)
echo "Total time: $((end_time - st))s"
