#!/bin/sh


# Directory paths
for g in 3
do
  for t in 3 4 5
  do
    for a in 20
    do
      # base_dir="G3_T4_A25"
      base_dir="G${g}_T${t}_A${a}"
      root_dir="data/18_04"
      input_dir="${root_dir}/${base_dir}/random_inputs_${base_dir}"
      output_dir="${root_dir}/${base_dir}/graph_cbs_${base_dir}_new"
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
    done
  done
done