#!/bin/bash
#SBATCH --job-name=MM_init
#SBATCH --nodes=4
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=28
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --time=00:05:00

# set OpenMP environment variables
export OMP_NUM_THREADS=28
export OMP_PLACES=cores
export OMP_PROC_BIND=spread

# launch OpenMP code
srun --export=all -n 1 -c ${OMP_NUM_THREADS} ./MM_init