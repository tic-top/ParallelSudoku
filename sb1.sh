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

# 创建命名管道
mkfifo serial_in
mkfifo serial_out

# 启动serial程序，并让其从serial_in读入，从serial_out输出
./serial < serial_in > serial_out &
serial_pid=$!

st=$(date +%s)
line_count=0

# 从 CSV 文件中读取数据并处理
tail -n +2 "$input_csv" | while IFS=, read -r puzzle solution clues difficulty difficulty_range; do
    line_count=$((line_count + 1))
    puzzle=$(echo "$puzzle" | sed 's/\r//g' | tr -d '\n')
    solution=$(echo "$solution" | sed 's/\r//g' | tr -d '\n')
    clues=$(echo "$clues" | sed 's/\r//g' | tr -d '\n')
    difficulty=$(echo "$difficulty" | sed 's/\r//g' | tr -d '\n')
    difficulty_range=$(echo "$difficulty_range" | sed 's/\r//g' | tr -d '\n')

    start_time=$(date +%s%3N)
    # 将数独传给serial程序
    echo "$puzzle" > serial_in

    # 从serial程序的输出中读取结果
    read -r result_and_time < serial_out

    end_time=$(date +%s%3N)
    elapsed_time=$((end_time - start_time))

    # 假设输出格式为：result runtime
    result=$(echo "$result_and_time" | awk '{print $1}')
    runtime=$(echo "$result_and_time" | awk '{print $2}')
    echo "Runtime: $runtime, Esttime: ${elapsed_time}ms"

    # 将结果写入CSV文件
    echo "$puzzle,$solution,$clues,$difficulty,$difficulty_range,$result,$runtime" >> "$output_csv"
done

# 所有题目处理完后，通知serial程序退出
echo "exit" > serial_in

# 等待serial程序退出
wait $serial_pid

# 清理命名管道
rm serial_in serial_out

end_time=$(date +%s)
echo "Total time: $((end_time - st))s"
