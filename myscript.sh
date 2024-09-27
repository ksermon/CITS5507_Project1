#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=4G
#SBATCH --time=00:01:00
#SBATCH --output=kgs_Project1.out

export OMP_NUM_THREADS=8

cc -o Project1 -fopenmp ./Project1.c
srun -c 8 ./Project1