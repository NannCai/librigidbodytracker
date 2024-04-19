#!/bin/sh


# Directory paths

base_dir="G5_T4_A20"
input_dir="data/${base_dir}/random_inputs_${base_dir}"
output_dir="data/${base_dir}/graph_cbs_${base_dir}"
# input_dir="data/random_inputs"
# output_dir="data/graph_cbs3"
echo "input_dir: $input_dir"
echo "output_dir: $output_dir"

mkdir -p $output_dir

for input_file in $input_dir/*.txt
do
  base_name=$(basename $input_file .txt)

  # Construct the output file name
  output_file="${output_dir}/graph_${base_name}.yaml"

  # Execute the assignment algorithm
  ./build/cbs_group_constraint -i $input_file -o $output_file
done