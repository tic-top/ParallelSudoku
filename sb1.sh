#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)
# Set up batch job settings

#SBATCH --job-name=sudoku
#SBATCH --mail-type=BEGIN,END
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
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

# 读取csv文件并处理
tail -n +2 "$input_csv" | while IFS=, read -r puzzle solution clues difficulty difficulty_range; do
    echo "$puzzle,$solution,$clues,$difficulty,$difficulty_range"
    # # 当处理满1000条后就停止
    # if [ $line_count -ge 100 ]; then
    #     break
    # fi
    # line_count=$((line_count + 1))

    # # 将quizzes作为输入传给程序并使用并行数p
    # result_and_time=$(echo "$puzzle" | ./serial)

    # # 假设输出格式是：result runtime
    # result=$(echo "$result_and_time" | awk '{print $1}')
    # runtime=$(echo "$result_and_time" | awk '{print $2}')

    # # 将结果写入CSV文件
    # echo "$puzzle,$solution,$clues,$difficulty,$difficulty_range,$result,$runtime" >> "$output_csv"
done
