#!/bin/sh

# for i in $(seq 0 19)
# do
#   freq=$(echo "scale=2; 0.05 * $((i))" | bc)
#   output_dir="./data/output_add_remove_1208/add_point$((i))"
#   ./build/playclouds ./example/3d_8m_hand.yaml ../../data/3d_8m_mo1 $freq $output_dir
# done


./build/playclouds ./example/3d_8m_hand.yaml ../../data/3d_8m_mo1 0 ./data/runtime_part/noise_icp