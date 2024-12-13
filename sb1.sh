#!/bin/bash

#SBATCH --job-name=sudoku
#SBATCH --mail-type=BEGIN,END
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem-per-cpu=1g
#SBATCH --time=00:05:00
#SBATCH --account=cse587f24_class
#SBATCH --partition=standard

module load gcc
g++ -O3 serial.cpp -o serial

input_csv="sudoku.csv"
output_csv="output1.csv"

echo "puzzle,solution,clues,difficulty,difficulty_range,result,runtime" > "$output_csv"

mkfifo serial_in
mkfifo serial_out

# 使用多种stdbuf参数确保无缓冲或行缓冲输出
stdbuf -i0 -oL -eL ./serial < serial_in > serial_out &
serial_pid=$!

start_total=$(date +%s)
line_count=0

while IFS=, read -r puzzle solution clues difficulty difficulty_range; do
    echo "Processing line $line_count..."
    line_count=$((line_count + 1))

    puzzle=$(echo "$puzzle" | tr -d '\r\n')
    solution=$(echo "$solution" | tr -d '\r\n')
    clues=$(echo "$clues" | tr -d '\r\n')
    difficulty=$(echo "$difficulty" | tr -d '\r\n')
    difficulty_range=$(echo "$difficulty_range" | tr -d '\r\n')

    start_time=$(date +%s%3N)

    # 将数独发送给serial程序
    echo "$puzzle" > serial_in

    # 从serial_out读取一行结果
    if ! read -r result_and_time < serial_out; then
        echo "Error: Failed to read from serial_out"
        break
    fi

    end_time=$(date +%s%3N)
    elapsed_time=$((end_time - start_time))

    result=$(echo "$result_and_time" | awk '{print $1}')
    runtime=$(echo "$result_and_time" | awk '{print $2}')

    echo "Runtime (solver): $runtime ms, Local: ${elapsed_time}ms"
    echo "$puzzle,$solution,$clues,$difficulty,$difficulty_range,$result,$runtime" >> "$output_csv"

done < <(tail -n +2 "$input_csv")

echo "exit" > serial_in
wait $serial_pid

rm serial_in serial_out

end_total=$(date +%s)
echo "Total time: $((end_total - start_total))s"
