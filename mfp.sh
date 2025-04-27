#!/bin/bash

# MFP Shell Script Interface
# This script provides a simple interface to the MFP CLI program
# Copyright (c) 2025 Daniel Charboneau. All rights reserved.

# Default values
METHOD=3
CPU=0

# Function to display usage
function show_usage {
    echo "Usage: $0 [OPTIONS] NUMBER|RANGE"
    echo "Options:"
    echo "  -m METHOD   Specify the factorization method (1, 2, or 3, default: 3)"
    echo "  -c CPU      Specify the number of CPU cores to use (default: auto)"
    echo "  -h          Display this help message"
    echo ""
    echo "Examples:"
    echo "  $0 123456789                # Factorize a single number using default method and CPU count"
    echo "  $0 -m 1 123456789           # Factorize a single number using method 1"
    echo "  $0 -m 2 -c 4 123456789      # Factorize a single number using method 2 with 4 CPU cores"
    echo "  $0 100-200                  # Factorize numbers in range 100 to 200 using default method"
    echo "  $0 -m 3 -c 8 1000-1010      # Factorize numbers in range 1000 to 1010 using method 3 with 8 CPU cores"
}

# Parse command-line options
while getopts ":m:c:h" opt; do
    case $opt in
        m)
            METHOD=$OPTARG
            if [[ ! $METHOD =~ ^[1-3]$ ]]; then
                echo "Error: Method must be 1, 2, or 3"
                exit 1
            fi
            ;;
        c)
            CPU=$OPTARG
            if [[ ! $CPU =~ ^[0-9]+$ ]]; then
                echo "Error: CPU count must be a non-negative integer"
                exit 1
            fi
            ;;
        h)
            show_usage
            exit 0
            ;;
        \?)
            echo "Error: Invalid option: -$OPTARG"
            show_usage
            exit 1
            ;;
        :)
            echo "Error: Option -$OPTARG requires an argument"
            show_usage
            exit 1
            ;;
    esac
done

# Shift the options so $1 is the first non-option argument
shift $((OPTIND-1))

# Check if a number or range is provided
if [ $# -ne 1 ]; then
    echo "Error: Must specify exactly one number or range"
    show_usage
    exit 1
fi

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Check if the argument is a range or a single number
if [[ $1 =~ ^([0-9]+)-([0-9]+)$ ]]; then
    # It's a range
    START=${BASH_REMATCH[1]}
    END=${BASH_REMATCH[2]}
    
    # Validate the range
    if [ $START -gt $END ]; then
        echo "Error: Range start must be less than or equal to range end"
        exit 1
    fi
    
    # Run the CLI program with the range
    "$SCRIPT_DIR/build/mfp_cli" --range $START $END --method $METHOD --cpu $CPU
else
    # It's a single number
    NUMBER=$1
    
    # Validate the number
    if [[ ! $NUMBER =~ ^[0-9]+$ ]]; then
        echo "Error: Invalid number format"
        exit 1
    fi
    
    # Run the CLI program with the single number
    "$SCRIPT_DIR/build/mfp_cli" --number $NUMBER --method $METHOD --cpu $CPU
fi
