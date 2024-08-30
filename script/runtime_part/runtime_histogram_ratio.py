# TODO use noise=0 data plot the histogram of runtime of each frame

# import numpy as np
import os
import matplotlib.pyplot as plt 


def parse_rigidbody_data(rigidbody_path):
    with open(rigidbody_path, 'r') as trans_file:
        rigidbody_data = trans_file.read().split(sep='\n')
    # print('Transformation Data:', rigidbody_data)

    runtime_dict = {}
    for line in rigidbody_data:
        if line.startswith('stamp:'):
            key = line
            runtime_dict[key] = {}
        elif line.startswith('Runtime:'):
            # print('runtime line',line)
            # runtime_dict[key] = line

            runtime_value = float(line.split()[1])  # Get the number from the line
            # print('runtime_value',runtime_value)
            runtime_dict[key]['Runtime'] = runtime_value  # Store the number instead of the full line
        elif line.startswith('dataset duration:'):
            dataset_runtime_value = float(line.split()[2])
            runtime_dict[key]['dataset'] = dataset_runtime_value  # Store the number instead of the full line

        elif line.startswith('cbs duration:'):
            cbs_runtime_value = float(line.split()[2])
            runtime_dict[key]['cbs'] = cbs_runtime_value  # Store the number instead of the full line

    # print("runtime_dict",runtime_dict)
    return runtime_dict

def runtime_his_ratio(rigidbody_path):
    runtime_dict = parse_rigidbody_data(rigidbody_path)

    # Extract data for histograms
    runtime_values = [v['Runtime'] for v in runtime_dict.values()]
    dataset_values = [v['dataset'] for v in runtime_dict.values()]
    cbs_values = [v['cbs'] for v in runtime_dict.values()]
    
    avg_runtime = sum(runtime_values) / len(runtime_values)
    avg_dataset = sum(dataset_values) / len(dataset_values)
    avg_cbs = sum(cbs_values) / len(cbs_values)

    dataset_ratios = [v['dataset'] / v['Runtime'] for v in runtime_dict.values()]
    cbs_ratios = [v['cbs'] / v['Runtime'] for v in runtime_dict.values()]

    fig, axs = plt.subplots(3, 1, figsize=(20, 15))
    plt.subplots_adjust(hspace=0.4, top=0.95, bottom=0.05, left=0.05, right=0.95)  # Adjust spacing

    # First histogram: Runtime values
    counts, bins, patches = axs[0].hist(runtime_values, bins=100, edgecolor='black')
    total_values = len(runtime_values)
    axs[0].set_title(f'Histogram of Runtime Values (Total: {total_values}, Avg: {avg_runtime:.6f})\nPath: {rigidbody_path}')
    axs[0].set_xlabel('Runtime')
    axs[0].set_ylabel('Frequency')
    axs[0].set_yscale('log')
    for count, x in zip(counts, bins):
        axs[0].text(x, count, str(int(count)), ha='center', va='bottom')

    # Second histogram: Dataset/Runtime ratio
    counts, bins, patches = axs[1].hist(dataset_ratios, bins=100, edgecolor='black')
    total_values = len(dataset_ratios)
    axs[1].set_title(f'Histogram of Dataset/Runtime Ratios (Total: {total_values}, Avg Dataset: {avg_dataset:.6f})')
    axs[1].set_xlabel('Dataset/Runtime Ratio')
    axs[1].set_ylabel('Frequency')
    axs[1].set_yscale('log')
    for count, x in zip(counts, bins):
        axs[1].text(x, count, str(int(count)), ha='center', va='bottom')

    # Third histogram: CBS/Runtime ratio
    counts, bins, patches = axs[2].hist(cbs_ratios, bins=100, edgecolor='black')
    total_values = len(cbs_ratios)
    axs[2].set_title(f'Histogram of CBS/Runtime Ratios (Total: {total_values}, Avg CBS: {avg_cbs:.6f})')
    axs[2].set_xlabel('CBS/Runtime Ratio')
    axs[2].set_ylabel('Frequency')
    axs[2].set_yscale('log')
    for count, x in zip(counts, bins):
        axs[2].text(x, count, str(int(count)), ha='center', va='bottom')

    # Print the number of values in each bin for each histogram
    for count, bin_edge in zip(counts, bins):
        print(f'Bin edge: {bin_edge}, Count: {count}')

    # plt.tight_layout()
    # plt.show()
    png_path = os.path.splitext(rigidbody_path)[0] + '.png'
    print('png_path',png_path)
    plt.savefig(png_path)






    # # Plotting the histogram
    # counts, bins, patches = plt.hist(runtime_dict.values(), bins=100, edgecolor='black')  # Capture counts and bins
    # # plt.title('Histogram of Runtime Values')  # Add this line
    # total_values = len(runtime_dict)  # Calculate total number of values
    # # plt.title(f'Histogram of Runtime Values (Total: {total_values})')  # Update title with total
    # plt.title(f'Histogram of Runtime Values (Total: {total_values})\nPath: {rigidbody_path}')  # Update title with total and path
    # plt.xlabel('Runtime')  # Add this line
    # plt.ylabel('Frequency')  # Add this line
    # plt.yscale('log')  # Set y-axis to logarithmic scale

    # # Annotate the number of values in each bin
    # for count, x in zip(counts, bins):
    #     plt.text(x, count, str(int(count)), ha='center', va='bottom')  # Add this line

    # plt.show()  # Add this line    # Print the number of values in each bin


    # for count, bin_edge in zip(counts, bins):
    #     print(f'Bin edge: {bin_edge}, Count: {count}')  # Print bin edge and count
if __name__ == '__main__':

    # path = './data/output_add_point/add_point0.txt'
    # path = './data/output_add_point/add_point40.txt'
    # path = './data/output_add_remove_1208/add_point0.txt'
    # path = './data/output_add_remove_1208/test0.txt'
    # path = './data/output_remove/test0.txt'
    # rigidbody_path = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/runtime_part/noise_icp.txt'
    # rigidbody_path = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/runtime_part/test.txt' 
    # rigidbody_path = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/2608runtime/figure8_2d_5m1.txt'
    # rigidbody_path = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/2608runtime/figure8_2d_5m2.txt'
    # rigidbody_path = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/2608runtime/figure8_3d_8m1.txt'
    rigidbody_dir = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/2608runtime'
    rigidbody_file_names = [
        # 'figure8_3d_8m1',
        # 'figure8_2d_5m2',
        'figure8_3d_8m2',
		'figure8_4d_9m1f',
		'figure8_4d_9m2',
		'figure8_4d_9m3f',
		'figure8_4d_9m4',
		'figure8_4d_9m5',
		'figure8_4d_9m6f',
		'figure8_4d_9m7'
    ]

    for rigidbody_file_name in rigidbody_file_names:
        rigidbody_path = rigidbody_dir + '/' + rigidbody_file_name +'.txt'
        runtime_his_ratio(rigidbody_path)
    

