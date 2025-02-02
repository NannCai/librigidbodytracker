import json
import matplotlib.pyplot as plt


if __name__ == '__main__':

    # Load result_dict from JSON files
    hybrid_path = './script/noise/result_dict_hybrid.json'
    old_multi_path = './script/noise/result_dict_old_multi.json'


    with open(hybrid_path, 'r') as f:
        result_dict_hybrid = json.load(f)

    with open(old_multi_path, 'r') as f:
        result_dict_old_multi = json.load(f)

    # Extracting x and y values from result_dict_hybrid
    x_values_hybrid = []
    y_values_hybrid = []

    for file_name, data in result_dict_hybrid.items():
        plot_data = data['plot']
        x_values_hybrid.append(plot_data[0])
        y_values_hybrid.append(plot_data[1])

    # Extracting x and y values from result_dict_old_multi
    x_values_old_multi = []
    y_values_old_multi = []

    for file_name, data in result_dict_old_multi.items():
        plot_data = data['plot']
        x_values_old_multi.append(plot_data[0])
        y_values_old_multi.append(plot_data[1])

    # Plot the data
    plt.figure(figsize=(10, 6))
    plt.scatter(x_values_hybrid, y_values_hybrid, color='blue', label='Hybrid')
    plt.scatter(x_values_old_multi, y_values_old_multi, color='red', label='Old Multi')

    # Adding labels and title
    plt.xlabel('Total Removed Points')
    plt.ylabel('Correct Count Ratio')
    plt.title('Total Removed Points vs Correct Count Ratio')
    plt.legend()

    # Show the plot
    plt.grid(True)
    # plt.show()

    save_path = f'./script/noise/evaluate_noise_compare.png'
    plt.savefig(save_path, bbox_inches='tight')
    print('Save Path:',save_path)