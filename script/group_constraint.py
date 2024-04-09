import gurobipy as gp
from gurobipy import GRB
import os
import yaml
import time  
import numpy as np

def print_solution(model):
    # for var in model.getVars():
    #     if abs(var.x) > 1e-6:
    #         print("{0}: {1}".format(var.varName, var.x))
    print('Total matching score: {0}'.format(model.objVal))
    return None

def parse_data(data,additional_cost = 1e4,additional_group = 50):
    assignments_dic = {}
    all_tasks = []
    for line in data.split('\n'):
        parts = line.split()
        if len(parts) < 3:
            continue
        agent = 'a' + parts[0]
        cost = int(parts[1])
        tasks = ['t' + task for task in sorted(parts[2:])]  # handle multiple tasks
        all_tasks.extend(tasks)
        group = tuple(set(tasks))  # is it need to be unique elements in the group?
        # print('group',group)
        if len(group) < len(tasks):  # skip the duplicate numbers  TODO maybe not
            print('!!!len(group) < len(tasks)!',len(group), '<', len(tasks))
            continue
        group_str = '_'.join(sorted(group))
        assignments_dic[agent, group_str] = cost    
    agents = tuple(set(agent for agent, _ in assignments_dic.keys()))
    for agent in agents:
        assignments_dic[agent,'t' + str(additional_group)] = additional_cost
        additional_group = additional_group + 1
    groups = tuple(set(group for _, group in assignments_dic.keys()))
    tasks_list = tuple(set(all_tasks)) 
    return assignments_dic,agents,groups,tasks_list


def gurobi_algorithm(assignments_dic,agents, groups, tasks_list):
    # print('----assignments_dic\n',assignments_dic)
    if not assignments_dic:
        print("assignments_dic is empty. Exiting the function.")
        return None,None,None
    
    combinations, ms = gp.multidict(assignments_dic)    # keys and diction
    # print('----assignments_dic\n',assignments_dic)
    # print('combinations',combinations)

    m = gp.Model('RAP')
    m.setParam('OutputFlag', 0)
    x = m.addVars(combinations,vtype=GRB.BINARY, name="assign")

    groupsConstrs = m.addConstrs((x.sum('*',g) <= 1 for g in groups), 'groupsConstrs')  # one group can only be asigned to one agent
    agentsConstrs = m.addConstrs((x.sum(a,'*') == 1 for a in agents), 'agentsConstrs')  # one agent can only have one group

    for task in tasks_list:
        # print('task',task)
        comb_containing = [
            (agent, group) for (agent, group) in assignments_dic
            if any(t == task for t in group.split('_')) 
        ]
        # print('comb_containing',comb_containing)
        taskConstr = m.addConstr(gp.quicksum(x[a, g] for (a,g) in comb_containing) <= 1, task+'Constr')

    m.setObjective(x.prod(ms), GRB.MINIMIZE)

    # save model for inspection
    save_model = 0
    if save_model:
        output_dir = 'data/output'
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        output_file = os.path.join(output_dir, 'RAP.lp')
        m.write(output_file)

    m.optimize()

    print('m.status',m.status)
    # display optimal values of decision variables
    if m.SolCount > 0:
        print_solution(m)   
    else:
        print("No solution found")
    return m,x,combinations

def save_gurobi_res(model,x,combinations,res_dir,input_file_name,runtime,additional_cost=1e4,additional_group = 50):
    if model is None:
        output_data = {'cost': 0,
                       'assignment':None}
    else:
        output_data = {
            'cost': model.objVal,
            'assignment': {
                int(agent[1:]): int(group[1:]) if '_' not in group else ' '.join(str(int(t[1:])) for t in group.split('_'))   
                # int(agent[1:]): int(group[1:]) if '_' not in group else ' '.join(str(int(t[1:])) for t in group.split('_'))   
                # int(agent[1:]): ' '.join(str(int(t[1:])) for t in group.split('_'))   
                for agent, group in combinations if (abs(x[agent, group].x) > 1e-6)
            },
            'runtime':runtime
        }
        # remove_flag = 1
        # if remove_flag:
        #     print('output_data',output_data)
        for key, value in list(output_data['assignment'].items()):
            if isinstance(value, int) and int(value) >= additional_group:
            # if isinstance(value, str) and value.isdigit() and int(value) >= additional_group:
                del output_data['assignment'][key]
                output_data['cost'] = output_data['cost'] - additional_cost
        print('output_data',output_data)

    if not os.path.exists(res_dir):
        os.makedirs(res_dir)
    if 'txt' in input_file_name:
        input_file_name = input_file_name.replace('.txt', '')

    res_file_name = f'gurobi_{input_file_name}.yaml'
    res_file = os.path.join(res_dir, res_file_name)
    print('res_file',res_file)
    with open(res_file, 'w') as file:
        # file.write(f"cost: {output_data['cost']}\n")
        # del output_data['cost']  # Remove 'cost' from the dictionary before dumping to YAML
        yaml.dump(output_data, file, sort_keys=False)

def Gurobi(input_txt_name,data,gurobi_res_dir):
    start_time = time.time()
    assignments_dic, agents, groups, tasks_list = parse_data(data,additional_cost = 1e4)
    model,x,combinations = gurobi_algorithm(assignments_dic,agents, groups, tasks_list)
    # if model is None:
    #     return None
    end_time = time.time()  # End timing
    runtime = end_time - start_time
    save_gurobi_res(model,x,combinations,gurobi_res_dir,input_txt_name,runtime) 
    return runtime
    
if __name__ == '__main__':
    # input_dir = 'data/random_inputs'
    # gurobi_res_dir = 'data/gurobi_res3'

    max_group_num = 3
    max_group_size = 3
    input_dir = f'data/maxGroup_{max_group_num}_maxMarker_{max_group_size}/random_inputs_maxGroup_{max_group_num}_maxMarker_{max_group_size}'
    gurobi_res_dir = f'data/maxGroup_{max_group_num}_maxMarker_{max_group_size}/gurobi_maxGroup_{max_group_num}_maxMarker_{max_group_size}'

    input_files = os.listdir(input_dir)
    # input_files = ['random_14.txt']
    for input_file in input_files:
        input_file_path = os.path.join(input_dir, input_file)
        with open(input_file_path, 'r') as file:
            data = file.read()
        Gurobi(input_file,data,gurobi_res_dir)


    # runtime_list = []
    # for input_file in input_files:
    #     start_time = time.time()  # Start timing
    #     print(input_file,'--------------')  # random_7.txt
    #     input_file_path = os.path.join(input_dir, input_file)
    #     with open(input_file_path, 'r') as file:
    #         data = file.read()
    #     additional_cost = 1e4
    #     additional_group = 50
    #     assignments_dic,agents,groups,tasks_list = parse_data(data,
    #                                                           additional_cost = additional_cost,
    #                                                           additional_group = additional_group)
    #     # print('assignments_dic',assignments_dic)
    #     model,x,combinations = gurobi_algorithm(assignments_dic,agents, groups, tasks_list)

    #     end_time = time.time()  # End timing
    #     runtime = end_time - start_time
    #     print(f"Runtime of gurobi_algorithm: {runtime} seconds")
    #     gurobi_runtime_file = gurobi_res_dir +'/runtime.txt'
    #     with open(gurobi_runtime_file, 'a') as runtime_file:
    #         runtime_file.write(f"Input File: {input_file}\n")
    #         runtime_file.write(f"Runtime: {runtime} seconds\n")
    #         runtime_file.write("\n")
    #     runtime_list.append(runtime)
    #     save_gurobi_res(model,x,combinations,gurobi_res_dir,input_file,runtime,
    #                     additional_cost = additional_cost,
    #                     additional_group = additional_group)
        
    # average_runtime = np.mean(runtime_list)
    # frequency =  1/average_runtime
    # print(f"Average runtime of gurobi_algorithm: {average_runtime} seconds")
    # print(f'Frequency of !gurobi!: {frequency}')
