import gurobipy as gp
from gurobipy import GRB
import yaml


if __name__ == '__main__':
    assignments = []
    input_path = 'input2.txt'

    with open(input_path, 'r') as file:
        for line in file:
            parts = line.split()
            agent = 'a' + parts[0]
            cost = int(parts[1])
            task = 't' + parts[2]
            assignments.append((agent, task, cost))

    print(assignments)

    # Create a cost dictionary
    cost = {(agent, task): cost for agent, task, cost in assignments}
    print(cost)
    # quit()

    agents = set(agent for agent, _ in cost.keys())
    tasks = set(task for _, task in cost.keys())

    # Task requirements
    # requirements = {'t0': 1, 't1': 2, 't2': 1, 't3': 1}

    # Create a Gurobi model
    model = gp.Model('ResourceAssignmentProblem')

    # Create decision variables
    x = {}
    for agent in agents:
        for task in tasks:
            x[agent, task] = model.addVar(vtype=GRB.BINARY, name=f'x_{agent}_{task}')
    print(x)  # {('a1', 't3'):.....
    # quit()



    # Objective function: minimize the total cost
    model.setObjective(gp.quicksum(cost[agent, task] * x[agent, task] for agent in agents for task in tasks), GRB.MINIMIZE)

    # Constraints: each agent is assigned to exactly one task
    # Constraints: task requirements
    for agent in agents:
        for task in tasks:
            model.addConstr(gp.quicksum(x[agent, task] for task in tasks) <= 1, f'assignment_{agent}')
            model.addConstr(gp.quicksum(x[agent, task] for agent in agents) == 1, f'requirement_{task}')
             # model.addConstr(gp.quicksum(x[agent, task] for agent in agents) <= requirements[task], f'requirement_{task}')  # will

    # Optimize the model
    model.optimize()


    # Print the results
    if model.status == GRB.OPTIMAL:
        print('Optimal assignment:')
        for agent in agents:
            for task in tasks:
                if x[agent, task].x > 0.5:
                    print(f'{agent} is assigned to {task} with cost {cost[agent, task]}')
        print('Total cost:', model.objVal)
    else:
        print('No solution found')


    # Save the output to 1.yaml
    output_data = {
        'cost': model.objVal,
        'assignment': {int(agent[1:]): int(task[1:]) for agent in agents for task in tasks if x[agent, task].x > 0.5}
    }
    # print(output_data)

    with open('1.yaml', 'w') as file:
        file.write(f"cost: {output_data['cost']}\n")
        del output_data['cost']  # Remove 'cost' from the dictionary before dumping to YAML
        yaml.dump(output_data, file)