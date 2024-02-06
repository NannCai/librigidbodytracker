import random
from group_constraint import gurobi_algorithm
import os
import yaml

def parse_data(data):
    assignments_dic = {}
    all_tasks = []
    additional_group =  50
    additional_cost = 1e4
    for line in data.split('\n'):
        parts = line.split()
        if len(parts) < 3:
            continue
        agent = 'a' + parts[0]
        cost = int(parts[1])
        tasks = ['t' + task for task in sorted(parts[2:])]
        all_tasks.extend(tasks)
        # group = tuple(set(tasks))
        group = tuple(tasks)
        # print("len(group) ,len(tasks)",len(group), len(tasks))
        # if len(group) !=  len(tasks):
        #     print("len(group) !=  len(tasks)",len(group), len(tasks))
        group_str = '_'.join(group)
        assignments_dic[agent, group_str] = cost    
    agents = tuple(set(agent for agent, _ in assignments_dic.keys()))
    for agent in agents:
        assignments_dic[agent,'t' + str(additional_group)] = additional_cost
        additional_group = additional_group + 1
    groups = tuple(set(group for _, group in assignments_dic.keys()))
    tasks_list = tuple(set(all_tasks))
    return assignments_dic, agents, groups, tasks_list

def gen_data():
    num_agent = random.randint(1, 8)  # range(num_agent) is the agent name
    group_size = 2
    data = ''
    for a in range(num_agent):
        print('a',a)
        num_group = random.randint(1, 5)
        for g in range(num_group):
            # temp = ' '.join([str(random.randint(0, 9)) for _ in range(2)]) # tasks/group
            # # print(temp)
            # cost = random.randint(1, 100)
            # data = data + ' '.join([str(a),str(cost),temp]) +'\n'
            temp = ' '.join([str(random.randint(0, 9)) for _ in range(group_size)])  # Generate tasks with 't'
            cost = random.randint(1, 100)
            data += f'{a} {cost} {temp}\n'  # Use f-string for string formatting
    return data

def save_input_txt(save_dir,data):
    if not os.path.exists(save_dir):
        os.makedirs(save_dir)
    output_file_name =  f'random_{i}.txt'
    output_file_path = os.path.join(save_dir, output_file_name)
    print('output_file_path',output_file_path)
    with open(output_file_path, 'w') as file:
        file.write(data)
    return output_file_name

def save_gurobi_res(model,x,combinations,res_dir,input_file_name):
    # Create a dictionary to store the output data
    output_data = {
        'cost': model.objVal,
        'assignment': {
            # int(agent[1:]): int(group[1:]) if '_' not in group else ' '.join(str(int(t[1:])) for t in group.split('_'))   
            int(agent[1:]): ' '.join(str(int(t[1:])) for t in group.split('_'))   
            for agent, group in combinations if (abs(x[agent, group].x) > 1e-6)
        }
    }
    print('output_data',output_data)

    if not os.path.exists(res_dir):
        os.makedirs(res_dir)
    if 'txt' in input_file_name:
        input_file_name = input_file_name.replace('.txt', '')

    res_file_name = f'gurobi_{input_file_name}.yaml'
    res_file = os.path.join(res_dir, res_file_name)
    print('res_file',res_file)
    with open(res_file, 'w') as file:
        file.write(f"cost: {output_data['cost']}\n")
        del output_data['cost']  # Remove 'cost' from the dictionary before dumping to YAML
        yaml.dump(output_data, file)

if __name__ == '__main__':
    # save_dir = 'data/inputs'
    random_inputs_dir = 'data/random_inputs'
    res_dir = 'data/gurobi_res'

    for i in range(10):
        data = gen_data()
        print('data',data)
        print('----')
        input_txt_name = save_input_txt(random_inputs_dir,data)

        assignments_dic, agents, groups, tasks_list = parse_data(data)
        # print('assignments_dic, agents, groups, tasks_list',assignments_dic, agents, groups, tasks_list)
        model,x,combinations = gurobi_algorithm(assignments_dic,agents, groups, tasks_list)

        save_gurobi_res( model,x,combinations,res_dir,input_txt_name) 


        print('end')













