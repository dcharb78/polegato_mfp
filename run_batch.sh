#!/bin/bash

# Script to run MFP on numbers from 1 to 10 million using 30 cores
# Copyright (c) 2025 Daniel Charboneau. All rights reserved.

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Check if build directory exists, if not create it and build the project
if [ ! -d "$SCRIPT_DIR/build" ] || [ ! -f "$SCRIPT_DIR/build/mfp_cli" ]; then
    echo "Build directory or executable not found. Building the project..."
    mkdir -p "$SCRIPT_DIR/build"
    cd "$SCRIPT_DIR/build"
    cmake ..
    make
    if [ $? -ne 0 ]; then
        echo "Error: Failed to build the project."
        exit 1
    fi
fi

# Output files
OUTPUT_FILE="$SCRIPT_DIR/mfp_results.txt"
PROGRESS_FILE="$SCRIPT_DIR/mfp_progress.txt"

# Create or clear the output files
echo "MFP Factorization Results (1 to 10,000,000) using 30 cores" > "$OUTPUT_FILE"
echo "Started at: $(date)" >> "$OUTPUT_FILE"
echo "----------------------------------------" >> "$OUTPUT_FILE"

echo "MFP Factorization Progress" > "$PROGRESS_FILE"
echo "Started at: $(date)" >> "$PROGRESS_FILE"
echo "----------------------------------------" >> "$PROGRESS_FILE"

# Function to run the batch processing in the background
run_batch_processing() {
    # Start time
    START_TIME=$(date +%s)

    # Process in batches to avoid memory issues
    BATCH_SIZE=1000
    TOTAL_NUMBERS=10000000

    for ((start=1; start<=TOTAL_NUMBERS; start+=BATCH_SIZE)); do
        end=$((start + BATCH_SIZE - 1))
        if [ $end -gt $TOTAL_NUMBERS ]; then
            end=$TOTAL_NUMBERS
        fi
        
        echo "Processing batch $start to $end..." >> "$PROGRESS_FILE"
        
        # Run the CLI with the current batch
        "$SCRIPT_DIR/build/mfp_cli" --range $start $end --method 3 --cpu 30 >> "$OUTPUT_FILE" 2>&1
        
        # Add a separator between batches
        echo "----------------------------------------" >> "$OUTPUT_FILE"
        
        # Calculate and display progress
        progress=$(echo "scale=2; $end * 100 / $TOTAL_NUMBERS" | bc)
        elapsed=$(($(date +%s) - START_TIME))
        elapsed_formatted=$(printf "%02d:%02d:%02d" $((elapsed/3600)) $((elapsed%3600/60)) $((elapsed%60)))
        
        # Update progress file
        echo "Progress: $progress% completed. Elapsed time: $elapsed_formatted" >> "$PROGRESS_FILE"
        echo "Current position: $end of $TOTAL_NUMBERS" >> "$PROGRESS_FILE"
        echo "----------------------------------------" >> "$PROGRESS_FILE"
    done

    # End time
    END_TIME=$(date +%s)
    TOTAL_TIME=$((END_TIME - START_TIME))
    HOURS=$((TOTAL_TIME / 3600))
    MINUTES=$(((TOTAL_TIME % 3600) / 60))
    SECONDS=$((TOTAL_TIME % 60))

    # Add summary to the output file
    echo "Completed at: $(date)" >> "$OUTPUT_FILE"
    echo "Total time: ${HOURS}h ${MINUTES}m ${SECONDS}s" >> "$OUTPUT_FILE"

    # Add summary to the progress file
    echo "Completed at: $(date)" >> "$PROGRESS_FILE"
    echo "Total time: ${HOURS}h ${MINUTES}m ${SECONDS}s" >> "$PROGRESS_FILE"
    echo "Factorization complete. Results saved to $OUTPUT_FILE" >> "$PROGRESS_FILE"
}

# Run the batch processing in the background
run_batch_processing &

# Get the PID of the background process
BATCH_PID=$!

# Display information to the user
echo "Batch processing started in the background with PID: $BATCH_PID"
echo "Results are being saved to: $OUTPUT_FILE"
echo "Progress is being logged to: $PROGRESS_FILE"
echo ""
echo "You can monitor progress with:"
echo "  tail -f $PROGRESS_FILE"
echo ""
echo "You can monitor results with:"
echo "  tail -f $OUTPUT_FILE"
echo ""
echo "To stop the process:"
echo "  kill $BATCH_PID"
