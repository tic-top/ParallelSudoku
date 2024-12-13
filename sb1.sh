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

# Compile the serial code
g++ -O3 serial.cpp -o serial

input_csv="sudoku.csv"
output_csv="output1.csv"

# 输出CSV表头
echo "puzzle,solution,clues,difficulty,difficulty_range,result,runtime" > "$output_csv"

# 创建命名管道
mkfifo serial_in
mkfifo serial_out

# 启动serial程序（使用stdbuf确保行缓冲）
stdbuf -oL ./serial < serial_in > serial_out &
serial_pid=$!

start_total=$(date +%s)
line_count=0

# 从CSV中读取数据(跳过表头)
tail -n +2 "$input_csv" | while IFS=, read -r puzzle solution clues difficulty difficulty_range; do
    echo "Processing line $line_count..."
    line_count=$((line_count + 1))

    # 清洗数据中的可能的回车换行符
    puzzle=$(echo "$puzzle" | tr -d '\r\n')
    solution=$(echo "$solution" | tr -d '\r\n')
    clues=$(echo "$clues" | tr -d '\r\n')
    difficulty=$(echo "$difficulty" | tr -d '\r\n')
    difficulty_range=$(echo "$difficulty_range" | tr -d '\r\n')

    start_time=$(date +%s%3N)
    # 将数独题目发送给serial进程
    echo "$puzzle" > serial_in

    # 从serial_out读取结果，read命令会阻塞直到读到一行输出
    if ! read -r result_and_time < serial_out; then
        echo "Error: Failed to read from serial_out"
        break
    fi

    end_time=$(date +%s%3N)
    elapsed_time=$((end_time - start_time))

    # 假设输出格式为"result runtime"
    result=$(echo "$result_and_time" | awk '{print $1}')
    runtime=$(echo "$result_and_time" | awk '{print $2}')

    echo "Runtime (from solver): $runtime ms, Local measured: ${elapsed_time}ms"

    # 写入结果到CSV
    echo "$puzzle,$solution,$clues,$difficulty,$difficulty_range,$result,$runtime" >> "$output_csv"

done

# 所有题目处理完后，通知serial退出
echo "exit" > serial_in

# 等待serial程序结束
wait $serial_pid

# 清理命名管道
rm serial_in serial_out

end_total=$(date +%s)
echo "Total time: $((end_total - start_total))s"
