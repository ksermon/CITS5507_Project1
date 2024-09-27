#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=28
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=4G
#SBATCH --time=00:01:00

cc -o Project1 -fopenmp ./Project1.c
srun ./Project1