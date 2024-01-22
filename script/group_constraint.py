import gurobipy as gp
from gurobipy import GRB
# from gurobipy import *
import yaml

def print_solution(model):
    for var in model.getVars():
        if abs(var.x) > 1e-6:
            print("{0}: {1}".format(var.varName, var.x))
    print('Total matching score: {0}'.format(model.objVal))
    return None

def parse_data(input_path):
    assignments_dic = {}
    all_tasks = []
    additional_group =  50
    additional_cost = 1e4
    with open(input_path, 'r') as file:
        for line in file:
            parts = line.split()
            agent = 'a' + parts[0]
            cost = int(parts[1])
            tasks = ['t' + task for task in parts[2:]]  # handle multiple tasks
            all_tasks.extend(tasks)
            group = tuple(set(tasks))  # is it need to be unique elements in the group?
            # print('group',group)
            if len(group) < len(tasks):
                print('!!!len(group) < len(tasks)!',len(group), '<', len(tasks))
            group_str = '_'.join(group)
            assignments_dic[agent, group_str] = cost    
    agents = tuple(set(agent for agent, _ in assignments_dic.keys()))
    for agent in agents:
        assignments_dic[agent,'t' + str(additional_group)] = additional_cost
        additional_group = additional_group + 1
    groups = tuple(set(group for _, group in assignments_dic.keys()))
    tasks_list = tuple(set(all_tasks)) # TODO all the element in group
    return assignments_dic,agents,groups,tasks_list

if __name__ == '__main__':
    input_path = 'input_group3.txt'
    # input_path = 'input2.txt'

    assignments_dic,agents,groups,tasks_list = parse_data(input_path)
    print('assignments_dic',assignments_dic)
    # quit()

    combinations, ms = gp.multidict(assignments_dic)    # keys and diction
    # print('----assignments_dic\n',assignments_dic)
    # print('combinations',combinations)

    m = gp.Model('RAP')
    x = m.addVars(combinations,vtype=GRB.BINARY, name="assign")

    groupsConstrs = m.addConstrs((x.sum('*',g) <= 1 for g in groups), 'groupsConstrs')  # one group can only be asigned to one agent
    agentsConstrs = m.addConstrs((x.sum(a,'*') == 1 for a in agents), 'agentsConstrs')  # one agent can only have one group

    # TODO  iteratively have less solution

    for task in tasks_list:
        print('task',task)
        comb_containing = [
            (agent, group) for (agent, group) in assignments_dic
            if any(t == task for t in group.split('_')) 
        ]
        print('comb_containing',comb_containing)
        taskConstr = m.addConstr(gp.quicksum(x[a, g] for (a,g) in comb_containing) <= 1, task+'Constr')

    m.setObjective(x.prod(ms), GRB.MINIMIZE)

    # save model for inspection
    m.write('output/RAP.lp')
    m.optimize()

    print('m.status',m.status)
    # display optimal values of decision variables
    if m.SolCount > 0:
        print_solution(m)   
    else:
        print("No solution found")
