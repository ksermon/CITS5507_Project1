#!/bin/bash
#SBATCH --job-name=kgs_Project1
#SBATCH --nodes=1
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --time=00:05:00
#SBATCH --output=kgs_Project1.out
#SBATCH --mem=4G

# set OpenMP environment variables
export OMP_NUM_THREADS=1

# launch OpenMP code
srun -N 1 -n 1 -c ${OMP_NUM_THREADS} ./Project1