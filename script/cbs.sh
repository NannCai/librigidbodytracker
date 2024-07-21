#!/bin/sh

# Define the root directory for random data
save_path="data"

# Define the maximum number of groups
group=5

# Define the lists of maximum number of tasks and agents
# task_list="3 4"
# agent_list="5 10 15"

task_list="5"
agent_list="15"


# Loop through each combination of max_task_num and max_agent_num
for task in $task_list; do
    for agent in $agent_list; do
        base_dir="G${group}_T${task}_A${agent}"
        input_dir="${save_path}/${base_dir}/random_data_${base_dir}"
        output_dir="${save_path}/${base_dir}/cbs_${base_dir}"
        
        echo "input_dir: $input_dir"
        echo "output_dir: $output_dir"

        # Remove the existing output directory if it exists
        if [ -d "$output_dir" ]; then
            echo "Removing the previous output_dir: $output_dir"
            rm -rf "$output_dir"
        fi

        mkdir -p $output_dir

        for input_file in $input_dir/*.txt
        do
            base_name=$(basename "$input_file" .txt)
            echo "base_name: $base_name"
            # Construct the output file name
            output_file="${output_dir}/cbs_${base_name}.yaml"

            # Execute the assignment algorithm
            ./build/cbs_group_constraint -i "$input_file" -o "$output_file"
        done
    done
done
