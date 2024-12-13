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

module load gcc
module load openmpi
g++ serial.cpp -o serial
echo 000000010400000000020000000000050407008000300001090000300400200050100000000806000 | ./serial 
echo 061490020280007050003108007600704031000250074090600000000010008570000206800906000 | ./serial
    
