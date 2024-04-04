#!/bin/sh


# Directory paths
input_dir="data/random_inputs"
output_dir="data/graph_cbs3"
echo "input_dir: $input_dir"
echo "output_dir: $output_dir"

mkdir -p $output_dir
input_file="data/random_inputs/random_0.txt"

base_name=$(basename $input_file .txt)

# Construct the output file name
output_file="${output_dir}/graph_${base_name}.yaml"

# Execute the assignment algorithm
./build/cbs_group_constraint -i $input_file -o $output_file
