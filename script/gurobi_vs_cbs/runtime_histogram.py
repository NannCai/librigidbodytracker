# TODO use noise=0 data plot the histogram of runtime of each frame

import numpy as np

import matplotlib.pyplot as plt 


def parse_rigidbody_data(rigidbody_path):
    with open(rigidbody_path, 'r') as trans_file:
        rigidbody_data = trans_file.read().split(sep='\n')
    # print('Transformation Data:', rigidbody_data)

    runtime_dict = {}
    for line in rigidbody_data[:]:
        if line.startswith('stamp:'):
            key = line
        elif line.startswith('Runtime:'):
            # print('runtime line',line)
            # runtime_dict[key] = line


            # Extract only the runtime number and store it
            runtime_value = float(line.split()[1])  # Get the number from the line
            print('runtime_value',runtime_value)
            runtime_dict[key] = runtime_value  # Store the number instead of the full line
    # print("runtime_dict",runtime_dict)
    # return runtime_dict

    # Plotting the histogram
    counts, bins, patches = plt.hist(runtime_dict.values(), bins=100, edgecolor='black')  # Capture counts and bins
    # plt.title('Histogram of Runtime Values')  # Add this line
    total_values = len(runtime_dict)  # Calculate total number of values
    # plt.title(f'Histogram of Runtime Values (Total: {total_values})')  # Update title with total
    plt.title(f'Histogram of Runtime Values (Total: {total_values})\nPath: {rigidbody_path}')  # Update title with total and path
    plt.xlabel('Runtime')  # Add this line
    plt.ylabel('Frequency')  # Add this line
    plt.yscale('log')  # Set y-axis to logarithmic scale

    # Annotate the number of values in each bin
    for count, x in zip(counts, bins):
        plt.text(x, count, str(int(count)), ha='center', va='bottom')  # Add this line

    plt.show()  # Add this line    # Print the number of values in each bin


    for count, bin_edge in zip(counts, bins):
        print(f'Bin edge: {bin_edge}, Count: {count}')  # Print bin edge and count

if __name__ == '__main__':

    # path = './data/output_add_point/add_point0.txt'
    # path = './data/output_add_point/add_point40.txt'
    # path = './data/output_add_remove_1208/add_point0.txt'
    # path = './data/output_add_remove_1208/test0.txt'
    # path = './data/output_remove/test0.txt'
    # path = './data/output_remove/test20.txt'
    path = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/runtime_part/noise_icp.txt'
    parse_rigidbody_data(path)