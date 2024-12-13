#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)
# Set up batch job settings

#SBATCH --job-name=sudoku
#SBATCH --mail-type=BEGIN,END
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=2
#SBATCH --mem-per-cpu=1g
#SBATCH --time=00:05:00
#SBATCH --account=cse587f24_class
#SBATCH --partition=standard

p=2
module load gcc
module load openmpi
mpic++ -O3 parallel.cpp -o main

export UCX_LOG_LEVEL=error

input_csv="sudoku.csv"
output_csv="output$p.csv"

# 输出CSV表头
echo "puzzle,solution,clues,difficulty,difficulty_range,result,runtime" > "$output_csv"

line_count=0
start_time=$(date +%s)
tail -n +2 "$input_csv" | while IFS=, read -r puzzle solution clues difficulty difficulty_range; do
    # 当处理满1000条后就停止
    # if [ $line_count -ge 100 ]; then
    #     break
    # fi
    line_count=$((line_count + 1))
    puzzle=$(echo "$puzzle" | sed 's/\r//g' | tr -d '\n')
    solution=$(echo "$solution" | sed 's/\r//g' | tr -d '\n')
    clues=$(echo "$clues" | sed 's/\r//g' | tr -d '\n')
    difficulty=$(echo "$difficulty" | sed 's/\r//g' | tr -d '\n')
    difficulty_range=$(echo "$difficulty_range" | sed 's/\r//g' | tr -d '\n')
    result_and_time=$(echo "$puzzle" | mpirun -n $p ./main)
    
    # 假设输出格式是：result runtime
    result=$(echo "$result_and_time" | awk '{print $1}')
    runtime=$(echo "$result_and_time" | awk '{print $2}')
    elapsed_time=$((end_time - start_time))  # Get elapsed time in seconds
    elapsed_time_ms=$((elapsed_time * 1000))  # Convert to milliseconds
    echo "Runtime: $runtime, Esttime: ${elapsed_time_ms}ms"
    # 将四个字段写入新的CSV文件中
    echo "$puzzle,$solution,$clues,$difficulty,$difficulty_range,$result,$runtime" >> "$output_csv"
done
