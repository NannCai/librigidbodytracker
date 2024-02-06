import random
from group_constraint import gurobi_algorithm
import os
import yaml

def get_matched_files_dict(gurobi_dir,graph_dir,input_dir):
    gurobi_files = os.listdir(gurobi_dir)
    graph_files = os.listdir(graph_dir)
    input_files = os.listdir(input_dir)
    # print('gurobi_files',gurobi_files)
    # print('graph_files',graph_files)
    # print('input_files',input_files)

    match_dict = {}

    for input_file in input_files:
        file_name = os.path.splitext(input_file)[0]
        matching_graphs = [graph for graph in graph_files if file_name in graph]
        matching_gurobis = [gurobi for gurobi in gurobi_files if file_name in gurobi]
        
        if len(matching_graphs) == 1 and len(matching_gurobis) == 1:
            match_dict[file_name] = {
                'graph': matching_graphs[0],
                'gurobi': matching_gurobis[0]
            }
            print('file_name',file_name,'match_dict[file_name]',match_dict[file_name])
            # print(f'File Name: {file_name}, Matching Graphs: {matching_graphs}, Matching Gurobis: {matching_gurobis}')
        else:
            print(f'Match not found for {file_name}')
            quit()
    return match_dict

def parse_res(gurobi_dir,graph_dir,matching_gurobis,matching_graphs):
    res_gurobi_path = os.path.join(gurobi_dir, matching_gurobis)
    res_graph_path = os.path.join(graph_dir, matching_graphs)

    try:
        with open(res_gurobi_path, 'r') as gurobi_file, open(res_graph_path, 'r') as graph_file:
            res_gurobi = yaml.safe_load(gurobi_file)
            res_graph = yaml.safe_load(graph_file)
            return res_gurobi,res_graph
    except FileNotFoundError as e:
        print(f'Error: File not found - {e.filename}')
    except yaml.YAMLError as e:
        print(f'Error loading YAML file: {e}')
    except Exception as e:
        print(f'Unexpected error: {e}')
        quit()
    return None,None

if __name__ == '__main__':
    gurobi_dir = 'data/gurobi_res'  # TODO how to deal with group=t1_t1
    graph_dir = 'data/graph_res'
    input_dir = 'data/random_inputs'

    match_dict = get_matched_files_dict(gurobi_dir,graph_dir,input_dir)
    # print('match_dict', match_dict)

    # for value in match_dict.values():
    for key,value in match_dict.items():
        print('key',key,'value',value)
        matching_gurobis = value.get('gurobi')
        matching_graphs = value.get('graph')
        # if not matching_gurobis or not matching_graphs:
        #     print("Error: 'gurobi' or 'graph' key not found in dictionary.")
        #     continue

        res_gurobi,res_graph = parse_res(gurobi_dir,graph_dir,matching_gurobis,matching_graphs)
        print('res_gurobi',res_gurobi)
        print('res_graph',res_graph)

        # Compare the content of res_gurobi and res_graph
        if res_gurobi['cost'] == res_graph['cost']:
            print("Content of 'res_gurobi' and 'res_graph' is the same.")
            print('!!!!!!')
        else:
            print("Content of 'res_gurobi' and 'res_graph' is different.")
            # record the different input and also all the outputs
            
            save_dir = 'data/evaluation'
            if not os.path.exists(save_dir):
                os.makedirs(save_dir)
            filename = f'{save_dir}/eval_{key}.txt'
            with open(filename, 'w') as file:
                file.write(f"res_gurobi:\n{res_gurobi}\n")
                file.write(f"res_graph:\n{res_graph}\n")
                with open(f'{input_dir}/{key}.txt', 'r') as input_file:
                    file.write(f"content in {key}.txt:\n{input_file.read()}\n")
    print('end')








