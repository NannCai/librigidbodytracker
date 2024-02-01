#!/bin/sh

# source ~/codes/mine/emba_ws/devel/setup.bash
# Set the base path
# base_path="/home/nancai/emvs_ws/src/rpg_emvs/evimo_data/"
# base_path="/mnt/HD4TB/shuang/datasets/EVIMO2/samsung_mono/sfm/eval"

# Directory paths
input_dir="data/random_inputs"
output_dir="data/graph_res"
echo "input_dir: $input_dir"
echo "output_dir: $output_dir"

# Ensure output directory exists
mkdir -p $output_dir


# Loop over all input files in the directory
for input_file in $input_dir/*.txt
do
  # Extract the base name of the file (without path and extension)
  base_name=$(basename $input_file .txt)

  # Construct the output file name
  output_file="${output_dir}/graph_${base_name}.yaml"

  # Execute the assignment algorithm
  ./build/assignment -i $input_file -o $output_file
done










# # Create an array to store processed folders
# # processed_folders=()

# # Iterate through the folders
# for folder in "$base_path"/*; do
#     if [ -d "$folder" ]; then
#         echo "Processing folder: $folder"
#         python3 ./evimo2bag.py --path "$folder"
#         # processed_folders+=("$folder")
#     fi
# done