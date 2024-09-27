#!/bin/bash
#SBATCH --nodes=4
#SBATCH --ntasks=1
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=4G
#SBATCH --time=00:01:00
#SBATCH --output=kgs_Project1.out

cc -o Project1 -fopenmp ./Project1.c
srun ./Project1