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

# 使用 coproc 启动 serial 程序并保持其运行
coproc SER { ./serial; }

st=$(date +%s)
line_count=0

# 从 CSV 文件中读取数据并逐条处理
tail -n +2 "$input_csv" | while IFS=, read -r puzzle solution clues difficulty difficulty_range; do
    line_count=$((line_count + 1))
    puzzle=$(echo "$puzzle" | sed 's/\r//g' | tr -d '\n')
    solution=$(echo "$solution" | sed 's/\r//g' | tr -d '\n')
    clues=$(echo "$clues" | sed 's/\r//g' | tr -d '\n')
    difficulty=$(echo "$difficulty" | sed 's/\r//g' | tr -d '\n')
    difficulty_range=$(echo "$difficulty_range" | sed 's/\r//g' | tr -d '\n')

    start_time=$(date +%s%3N)
    # 将数独传给已运行的serial程序
    echo "$puzzle" >&"${SER[1]}"

    # 从serial程序读回结果
    # 假设serial在输出完结果后立即换行，所以我们可以直接read
    read -r result_and_time <&"${SER[0]}"

    end_time=$(date +%s%3N)
    elapsed_time=$((end_time - start_time))

    # 假设输出格式为：result runtime
    result=$(echo "$result_and_time" | awk '{print $1}')
    runtime=$(echo "$result_and_time" | awk '{print $2}')

    echo "Runtime: $runtime, Esttime: ${elapsed_time}ms"

    # 将结果写入CSV文件
    echo "$puzzle,$solution,$clues,$difficulty,$difficulty_range,$result,$runtime" >> "$output_csv"
done

# 所有题目处理完后，向serial程序发送exit指令，让其停止
echo "exit" >&"${SER[1]}"

# 等待serial程序退出
wait "${SER_PID}"

end_time=$(date +%s)
echo "Total time: $((end_time - st))s"
