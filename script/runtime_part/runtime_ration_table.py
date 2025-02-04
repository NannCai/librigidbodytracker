# import os
import matplotlib.pyplot as plt 

import statistics

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

def histogram():


    avg_runtime = sum(runtime_values) / len(runtime_values)
    avg_dataset = sum(dataset_values) / len(dataset_values)
    avg_cbs = sum(cbs_values) / len(cbs_values)

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

    plt.show()


if __name__ == '__main__':
    # rigidbody_dir = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/2608runtime'
    # rigidbody_file_names = [
    #     'figure8_3d_8m2'
    # ]

    rigidbody_dir = '/home/nan/ros2_ws/src/motion_capture_tracking/motion_capture_tracking/deps/librigidbodytracker/data/030225runtime_grc'
    rigidbody_file_names = [
		'2d_8m',
        'figure8_3d_8m2',
		'4d_4m',
		'4d_10m',
		'4d_16m',
		'8d_20m',
		'8d_22m2',
    ]


    table_prefix = [
        ("2", "$2\\times4=8$"),
        ("3", "$1\\times4 + 1\\times3 + 1\\times1=8$"),
        ("4", "$4\\times1=4$"),
        ("4", "$2\\times4 + 2\\times1=10$"),
        ("4", "$4\\times4=16$"),
        ("8", "$4\\times4 + 4\\times1=20$"),
        ("8", "$4\\times4 + 1\\times3 + 3\\times1=22$")
    ]


    for i, rigidbody_file_name in enumerate(rigidbody_file_names):
        rigidbody_path = rigidbody_dir + '/' + rigidbody_file_name +'.txt'
        # print('rigidbody_file_name',rigidbody_file_name)

        runtime_dict = parse_rigidbody_data(rigidbody_path)
        runtime_values = [v['Runtime'] for v in runtime_dict.values()]
        dataset_values = [v['dataset'] for v in runtime_dict.values()]
        cbs_values = [v['cbs'] for v in runtime_dict.values()]


        dataset_ratios = [v['dataset'] / v['Runtime'] for v in runtime_dict.values()]
        cbs_ratios = [v['cbs'] / v['Runtime'] for v in runtime_dict.values()]

        dataset_cbs_ratios = [v['dataset'] / v['cbs'] for v in runtime_dict.values()]

        # Calculate median and maximum for runtime values
        # runtime_median = statistics.median(runtime_values)
        runtime_median_ms = statistics.median(runtime_values) * 1000
        # runtime_max = max(runtime_values)
        over_10ms_count = sum(1 for v in runtime_values if v > 0.01)
        percentage_over_10ms = (over_10ms_count / len(runtime_values)) * 100

        # Calculate median and maximum for dataset-to-CBS ratios
        # ratio_median = statistics.median(dataset_cbs_ratios)
        # ratio_max = max(dataset_cbs_ratios)

        cbs_contribution_ratios = [cbs / (cbs + dataset) * 100 
                                 for cbs, dataset in zip(cbs_values, dataset_values)]
        cbs_contribution_median = statistics.median(cbs_contribution_ratios)


        # print(f"Runtime: median = {runtime_median:.6f}, max = {runtime_max:.6f}")
        # print(f"Dataset_CBS_Ratios: median = {ratio_median:.3f}, max = {ratio_max:.3f}")
        


        # print(f"& {runtime_median_ms:.1f}    & {percentage_over_10ms:.1f}    & {ratio_median:.3f}    & {ratio_max:.3f}\\\\")
        prefix_num, prefix_expr = table_prefix[i]

        # print(f"& {runtime_median_ms:.2f}    & {percentage_over_10ms:.2f}\\%    & {cbs_contribution_median:.2f}\\%\\\\")
        print(f"{prefix_num} & {prefix_expr} & {runtime_median_ms:.2f} & {percentage_over_10ms:.2f}\\% & {cbs_contribution_median:.2f}\\%\\\\")

        # histogram()