#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)
# Set up batch job settings

#SBATCH --job-name=sudoku
#SBATCH --mail-type=BEGIN,END
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=8
#SBATCH --mem-per-cpu=1g
#SBATCH --time=00:05:00
#SBATCH --account=cse587f24_class
#SBATCH --partition=standard

p=8
module load gcc
module load openmpi
mpic++ -O3 parallel.cpp -o main

export UCX_LOG_LEVEL=error

input_csv="sudoku.csv"
output_csv="output$p.csv"
mpirun -n $p ./main < "$input_csv" > "$output_csv"