#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)
# Set up batch job settings

#SBATCH --job-name=sudoku
#SBATCH --mail-type=BEGIN,END
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=16
#SBATCH --mem-per-cpu=1g
#SBATCH --time=00:05:00
#SBATCH --account=cse587f24_class
#SBATCH --partition=standard

module load gcc
module load openmpi
mpic++ -O3 parallel.cpp -o main
export UCX_LOG_LEVEL=error
# echo 000000010400000000020000000000050407008000300001090000300400200050100000000806000 | mpirun -n 4 ./main
echo "pard" >> result.txt
start=`date +%s`
echo 000070030000003020056200089000064001007000000090080005900005000000037910280000000 | mpirun -n 16 ./main >> result.txt
end=`date +%s`
runtime=$((end-start))
echo "Elapse: $runtime" >> result.txt
