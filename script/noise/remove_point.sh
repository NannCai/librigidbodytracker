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



# for i in $(seq 0 19)
# do
#   freq=$(echo "scale=2; 0.05 * $((i))" | bc)
#   output_dir="./data/output_add_remove_1208/test$((i))"
#   ./build/playclouds ./example/3d_8m_hand.yaml ../../data/3d_8m_mo1 $freq $output_dir
# done


# for i in $(seq 0 19)
# do
#   freq=$(echo "scale=2; 0.01 * $((i+1))" | bc)
#   output_dir="./data/output_add_remove_1208/test$((i+40))"
#   ./build/playclouds ./example/3d_8m_hand.yaml ../../data/2807_24/3d_8m_mo1 $freq $output_dir
# done

for i in $(seq 0 19)
do
  freq=$(echo "scale=2; 0.05 * $((i))" | bc)
  output_dir="./data/output_add_remove_old_multi_mode_2d_7m_mo_0701/test$((i))"
  ./build/playclouds ./example/3d_8m_hand.yaml ../../data/2807_24/2d_7m_mo $freq $output_dir
done

for i in $(seq 0 19)
do
  freq=$(echo "scale=2; 0.01 * $((i+1))" | bc)
  output_dir="./data/output_add_remove_old_multi_mode_2d_7m_mo_0701/test$((i+20))"
  ./build/playclouds ./example/3d_8m_hand.yaml ../../data/2807_24/2d_7m_mo $freq $output_dir
done