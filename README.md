# ParallelSudoku
Fall 2024 EECS587 Final Project

mpirun -np [number of core] --bind-to core:overload-allowed ./main [m] [n] [verbose(1) or not(0)] [parallel(1) or serial(0)] > [output path]