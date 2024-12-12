#bin/bash
module load gcc
module load openmpi
mpic++ -O3 main.cpp -o main
sbatch serial.sh