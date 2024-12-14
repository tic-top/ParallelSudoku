#!/bin/bash

#SBATCH --job-name=sudoku_1
#SBATCH --mail-type=BEGIN,END
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem-per-cpu=1g
#SBATCH --time=00:05:00
#SBATCH --account=cse587f24_class
#SBATCH --partition=standard

module load gcc
g++ -O3 serial.cpp -o serial
output_csv="output1.csv"

start=$(date +%s)
./serial < sudoku.csv > "$output_csv"
end=$(date +%s)
echo "Execution time: $((end-start)) seconds"
