#!/bin/sh

# Define the root directory for random data
save_path="data"

# Define the maximum number of groups
group=5

# Define the lists of maximum number of tasks and agents
task_list="3 4 5"
agent_list="5 10 15 20"

# Loop through each combination of max_task_num and max_agent_num
for task in $task_list; do
    for agent in $agent_list; do
        echo "Running with max_task_num=$task and max_agent_num=$agent"
        python3 ./script/gurobi.py -p "$save_path" -g "$group" -t "$task" -a "$agent"
    done
done
