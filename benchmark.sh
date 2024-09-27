#!/bin/bash
#SBATCH --job-name=thread_benchmark
#SBATCH --output=thread_benchmark_%j.out
#SBATCH --error=thread_benchmark_%j.err
#SBATCH --time=00:30:00        # Adjust based on expected runtime
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=256    # Maximum number of threads to test
#SBATCH --mem=128G             # Adjust based on memory requirements

# Load necessary modules (adjust as per your environment)
module load gcc/9.3.0       # Example: GCC compiler with OpenMP support

# Define the range of threads to test
THREAD_COUNTS=(1 2 4 8 16 32 64 128 256)

# Probability and number of matrices
PROB=0.01
NUM_MATRICES=100000

# Output file to store results
RESULTS="benchmark_results.txt"
echo "Threads,Time(s)" > $RESULTS

echo "Starting benchmarking..."

for THREADS in "${THREAD_COUNTS[@]}"
do
    echo "Running with $THREADS threads..."
    
    # Run the program and capture the output
    OUTPUT=$(./Project1 $PROB $THREADS)
    
    # Extract the execution time from the program's output
    TIME=$(echo "$OUTPUT" | grep "Time taken to generate and process matrices" | awk '{print $7}')
    
    # Log the thread count and execution time
    echo "$THREADS,$TIME" >> $RESULTS
    
    echo "Completed with $THREADS threads in $TIME seconds."
done

echo "Benchmarking completed. Results saved to $RESULTS."
