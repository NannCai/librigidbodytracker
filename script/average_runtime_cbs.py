import os
import yaml

def runtime_txtfile():
    txt_path = 'data/graph_cbs3/runtime.txt'
    # Read the file and extract runtimes
    with open(txt_path, 'r') as file:
        runtimes = [float(line.split(': ')[1].split(' seconds')[0]) for line in file.readlines() if 'Runtime' in line]
        print('runtimes',runtimes)
    # Calculate the average runtime
    print('len(runtimes)',len(runtimes))
    average_runtime = sum(runtimes) / len(runtimes)
    frequency = 1/average_runtime
    print(f'Average Runtime: {average_runtime} seconds')

    print(f'Frequency of !CBS! Average Runtime: {frequency}')



if __name__ == '__main__':
    # runtime_txtfile()
    directory = 'data/graph_cbs3'
    runtimes = []
    runtimes_under30 = []
    runtimes_over30 = []

    for filename in os.listdir(directory):
        if filename.endswith(".yaml"):
            with open(os.path.join(directory, filename), 'r') as file:
                data = yaml.safe_load(file)
                th = 100
                if data['highLevelExpanded'] < th:
                    runtimes_under30.append(data['runtime'])
                else:
                    # runtimes_over30.append(data['runtime'])
                    print(filename,'runtime,loops',data['runtime'],data['highLevelExpanded'])
                runtimes.append(data['runtime'])
    # print('runtimes_over30',runtimes_over30)
    average_runtime_30loop = sum(runtimes_under30) / len(runtimes_under30)
    frequency_30loop = 1 / average_runtime_30loop
    print(f"Average runtime of cbs_assignment ({th} loop): {average_runtime_30loop} seconds")
    print(f'Frequency of cbs_assignment ({th} loop): {frequency_30loop}')
    print(f"under 30 loop: {len(runtimes_under30)}")

    average_runtime = sum(runtimes) / len(runtimes)
    frequency =  1/average_runtime
    print(f"Average runtime of cbs_assignment: {average_runtime} seconds")
    print(f'Frequency of !cbs_assignment!: {frequency}')

    print(f"all files: {len(runtimes)}")
