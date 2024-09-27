#!/bin/bash
#SBATCH --job-name=kgs_Project1
#SBATCH --nodes=1
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --time=00:01:00
#SBATCH --output=kgs_Project1.out
#SBATCH --mem=4G

# set OpenMP environment variables
export OMP_NUM_THREADS=16
export OMP_PLACES=cores
export OMP_PROC_BIND=spread

# launch OpenMP code
srun -N 1 -n 1 -c ${OMP_NUM_THREADS} --gres=gpu:1 --gpus-per-task=1 --gpu-bind=closest ./Project1