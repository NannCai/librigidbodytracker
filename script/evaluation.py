import random
from group_constraint import gurobi_algorithm
import os
import yaml

if __name__ == '__main__':
    gurobi_dir = 'data/gurobi_res'
    graph_dir = 'data/graph_res'
    input_dir = 'data/random_inputs'

    gurobi_files = os.listdir(gurobi_dir)
    graph_files = os.listdir(graph_dir)
    input_files = os.listdir(input_dir)
    # print('gurobi_files',gurobi_files)
    # print('graph_files',graph_files)
    # print('input_files',input_files)
    
    match_dict = {}
    
    for input_file in input_files:
        file_name, _ = os.path.splitext(input_file)
        matching_gurobis = [gurobi for gurobi in gurobi_files if file_name in gurobi]
        matching_graphs = [graph for graph in graph_files if file_name in graph]

        if len(matching_graphs) == 1 and len(matching_gurobis) == 1:
            res_gurobi_path = os.path.join(gurobi_dir, matching_gurobis[0])
            res_graph_path = os.path.join(graph_dir, matching_graphs[0])

            try:
                with open(res_gurobi_path, 'r') as file:
                    res_gurobi = yaml.safe_load(file)
                with open(res_graph_path, 'r') as file:
                    res_graph = yaml.safe_load(file)
                
                match_dict[file_name] = {
                    'graph': res_graph,
                    'gurobi': res_gurobi
                }
            except Exception as e:
                print(f'Error loading files for {file_name}: {e}')
                quit()
        else:
            print(f'Match not found for {file_name}')
            quit()



            
        # print('match_dict', match_dict)

    print('end')








