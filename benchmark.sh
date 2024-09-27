#!/bin/bash

#SBATCH --job-name=kgs_thread_benchmark    # Job name
#SBATCH --nodes=1                          # Maximum number of nodes
#SBATCH --account=courses0101              # Account name
#SBATCH --partition=work                   # Partition name
#SBATCH --output=output_%j.txt             # Output file
#SBATCH --error=error_%j.txt               # Error file
#SBATCH --ntasks=1                         # Number of tasks
#SBATCH --time=02:00:00                    # Maximum time limit

# Load OpenMP environment settings
module load gcc                             # Ensure gcc or another OpenMP-compatible compiler is loaded

# Set the list of thread values (1 to 256)
thread_values=(1 2 4 8 16 32 64 128 256)

# Set the list of probabilities
probabilities=(0.01 0.02 0.5)

# Export the OpenMP environment variable
export OMP_DYNAMIC=FALSE

# Loop over probabilities
for prob in "${probabilities[@]}"; do
    # Loop over thread values
    for threads in "${thread_values[@]}"; do
        export OMP_NUM_THREADS=$threads
        echo "Running with $threads threads and probability $prob"
        srun --cpus-per-task=$threads ./Project1 $threads $prob >> results_${prob}.txt
    done
done

echo "All benchmarks completed."