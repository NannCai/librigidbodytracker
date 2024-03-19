



if __name__ == '__main__':
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
