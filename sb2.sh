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

module load gcc
module load openmpi
mpic++ -O3 parallel.cpp -o main
export UCX_LOG_LEVEL=error

input_csv="sudoku.csv"
output_csv="output2.csv"

# 输出CSV表头
echo "quizzes,solutions,result,runtime" > "$output_csv"

line_count=0
tail -n +2 "$input_csv" | while IFS=, read -r quizzes solutions; do
    # 当处理满1000条后就停止
    if [ $line_count -ge 100 ]; then
        break
    fi
    line_count=$((line_count + 1))
    # quizzes和solutions是当前行的两列内容
    # 将quizzes作为输入给程序
    result_and_time=$(echo "$quizzes" | mpirun -n 2 ./main)
    
    # 假设输出格式是：result runtime
    result=$(echo "$result_and_time" | awk '{print $1}')
    runtime=$(echo "$result_and_time" | awk '{print $2}')

    # 将四个字段写入新的CSV文件中
    echo "$quizzes,$solutions,$result,$runtime" >> "$output_csv"
done
