#!/bin/sh

# run under ~/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker

# input_dir="../../data"
# output_dir="./data/2608runtime"
# yaml_dir="./example"


input_dir="/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/data/030225"
output_dir="./data/030225runtime_grc"
yaml_dir="./example/030225"

# for input_file in "$input_dir"/*; do
#     # Get the base name of the file (e.g., figure8_3d_8m1)
#     base_name=$(basename "$input_file")

#     # Set the output directory using the base name
#     output_file="$output_dir/$base_name"

#     yaml_file="$yaml_dir/$base_name.yaml"

#     echo "Processing input file: $input_file"
#     echo "Output directory: $output_file"


#     # Run the command with the current input file and output directory
#     # ./build/playclouds ./example/figure8_2d_5m1.yaml $input_file 0 $output_file
#     ./build/playclouds $yaml_file $input_file 0 $output_file
# done

input_file="/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/data/030225/figure8_3d_8m2"
# Get the base name of the file (e.g., figure8_3d_8m1)
base_name=$(basename "$input_file")

# Set the output directory using the base name
output_file="$output_dir/$base_name"

yaml_file="$yaml_dir/$base_name.yaml"

echo "Processing input file: $input_file"
echo "Output directory: $output_file"


# Run the command with the current input file and output directory
# ./build/playclouds ./example/figure8_2d_5m1.yaml $input_file 0 $output_file
./build/playclouds $yaml_file $input_file 0 $output_file
