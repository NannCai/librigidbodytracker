#!/bin/sh


# # Iterate over the range of values for the parameter
# for i in $(seq 0.01 0.01 0.19); do
#   # Iterate over the output directories
#   for j in $(seq 0 2); do
#     # Construct the output directory path
#     output_dir="./data/output/test$j"
#     # Run the command with the current values of the parameter and output directory
#     ./build/playclouds ./example/3d_8m_hand.yaml ../../data/3d_8m_mo1 $i $output_dir
#   done
# done


#!/bin/sh

# for i in $(seq 0 19)
# do
#   freq=$(echo "scale=2; 0.01 * $((i))" | bc)
#   output_dir="./data/output_add_point/add_point$((i))"
#   ./build/playclouds ./example/3d_8m_hand.yaml ../../data/3d_8m_mo1 $freq $output_dir
# done

for i in $(seq 0 19)
do
  freq=$(echo "scale=2; 0.05 * $((i))" | bc)
  output_dir="./data/output_add_remove_1208/add_point$((i))"
  ./build/playclouds ./example/3d_8m_hand.yaml ../../data/3d_8m_mo1 $freq $output_dir
done

for i in $(seq 0 19)
do
  freq=$(echo "scale=2; 0.05 * $((i+1))" | bc)
  output_dir="./data/output_add_remove_1208/add_point$((i+20))"
  ./build/playclouds ./example/3d_8m_hand.yaml ../../data/3d_8m_mo1 $freq $output_dir
done