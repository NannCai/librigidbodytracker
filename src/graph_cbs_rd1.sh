#!/bin/sh


# Directory paths
# input_dir="data/random_inputs"
# output_dir="data/graph_cbs3"

base_dir="G3_T3_A5"
root_dir="data/18_04"
input_dir="${root_dir}/${base_dir}/random_inputs_${base_dir}"
output_dir="${root_dir}/${base_dir}/graph_cbs_${base_dir}_new"
echo "input_dir: $input_dir"
echo "output_dir: $output_dir"

mkdir -p $output_dir
# input_file="data/random_inputs/random_20.txt"
for txt_num in 0
do 
    input_file="${input_dir}/random_${txt_num}.txt"

    base_name=$(basename $input_file .txt)

    # Construct the output file name
    output_file="${output_dir}/graph_${base_name}.yaml"

    # Execute the assignment algorithm
    ./build/cbs_group_constraint -i $input_file -o $output_file
done