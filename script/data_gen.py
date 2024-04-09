import random
# from group_constraint import gurobi_algorithm,save_gurobi_res,parse_data
from group_constraint import Gurobi

import os
import time  

def gen_data(max_group_num,max_group_size):
    num_agent = random.randint(1, 8)  # range(num_agent) is the agent name
    data = ''
    for a in range(num_agent):
        print('a',a)
        # group_size = 2
        num_group = random.randint(1, max_group_num)
        group_size = random.randint(1, max_group_size)  # how many tasks in a group
        for i in range(num_group):
            group = ' '.join([str(random.randint(0, 11)) for _ in range(group_size)])  # Generate tasks with 't'
            cost = random.randint(1, 100)
            data += f'{a} {cost} {group}\n'  # Use f-string for string formatting
    return data

def save_input_txt(save_dir,data,i):
    if not os.path.exists(save_dir):
        os.makedirs(save_dir)
    output_file_name =  f'random_{i}.txt'
    output_file_path = os.path.join(save_dir, output_file_name)
    print('output_file_path',output_file_path)
    with open(output_file_path, 'w') as file:
        file.write(data)
    return output_file_name

if __name__ == '__main__':
    # save_dir = 'data/inputs'
    max_group_num = 3
    max_group_size = 3
    random_inputs_dir = f'data/maxGroup_{max_group_num}_maxMarker_{max_group_size}/random_inputs_maxGroup_{max_group_num}_maxMarker_{max_group_size}'
    gurobi_res_dir = f'data/maxGroup_{max_group_num}_maxMarker_{max_group_size}/gurobi_maxGroup_{max_group_num}_maxMarker_{max_group_size}'
    for i in range(30):
        data = gen_data(max_group_num,max_group_size)
        # print('data',data)
        # print('----')
        input_txt_name = save_input_txt(random_inputs_dir,data,i)        

        runtime = Gurobi(input_txt_name,data,gurobi_res_dir)  # runtime without read the file and write the result into the file


        print('end')













