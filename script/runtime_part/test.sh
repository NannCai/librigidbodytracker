#!/bin/sh

# for i in $(seq 0 19)
# do
#   freq=$(echo "scale=2; 0.05 * $((i))" | bc)
#   output_dir="./data/output_add_remove_1208/add_point$((i))"
#   ./build/playclouds ./example/3d_8m_hand.yaml ../../data/3d_8m_mo1 $freq $output_dir
# done


./build/playclouds ./example/figure8_3d_8m2.yaml ../../data/figure8_3d_8m2 0 ./data/2608runtime/figure8_3d_8m2