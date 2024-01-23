import random
from group_constraint import gurobi_algorithm

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
        tasks = ['t' + task for task in parts[2:]]
        all_tasks.extend(tasks)
        group = tuple(set(tasks))
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
    # num_task = num_agent * 2 + random.randint(-1, 10)
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
    print('data',data)
    print('----')
    return data

if __name__ == '__main__':

    data = gen_data()
    assignments_dic, agents, groups, tasks_list = parse_data(data)
    print('assignments_dic, agents, groups, tasks_list',assignments_dic, agents, groups, tasks_list)
    
    gurobi_algorithm(assignments_dic,agents, groups, tasks_list)
    
    
    print('end')







