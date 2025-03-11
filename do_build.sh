#!/bin/bash

# Wrap to ckbuild.sh
export WP=${WP:-$(realpath $PWD/../)}

# Buffer the passed arguments
args="$@"

# Check if "u", "d", and "r" are in the arguments
contains_u=false
contains_d=false
contains_r=false

if [[ "$args" == *u* ]]; then
    contains_u=true
fi
if [[ "$args" == *d* ]]; then
    contains_d=true
fi
if [[ "$args" == *r* ]]; then
    contains_r=true
fi

if $contains_u && $contains_d; then
    echo "INFO: Will make a regular and unlocked build"
    if $contains_r; then
        echo "INFO: Two subsequent clean builds will be made"
        # Remove only "d" from arguments
        filtered_args="${args//d/}"
        bash kernel_build/ckbuild.sh $filtered_args
        # Remove "u" and "d" from arguments
        filtered_args="${filtered_args//u/}"
        bash kernel_build/ckbuild.sh $filtered_args
    else
        # Remove only "d" from arguments
        filtered_args="${args//d/}"
        bash kernel_build/ckbuild.sh $filtered_args
        # Remove "u" and "c" from arguments
        filtered_args="${filtered_args//u/}"
        filtered_args="${filtered_args//c/}"
        bash kernel_build/ckbuild.sh $filtered_args
    fi
elif $contains_d; then
    echo "INFO: Dual build argument ignored"
    # Remove "d" from arguments
    filtered_args="${args//d/}"
    bash kernel_build/ckbuild.sh $filtered_args
else
    bash kernel_build/ckbuild.sh "$@"
fi
