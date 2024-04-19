import os
import yaml
import matplotlib.pyplot as plt

def get_matched_files_dict(gurobi_dir,graph_dir,input_dir):
    gurobi_files = os.listdir(gurobi_dir)
    graph_files = os.listdir(graph_dir)
    input_files = os.listdir(input_dir)

    match_dict = {}
    for input_file in input_files:
        file_name = os.path.splitext(input_file)[0]
        # new_filename = filename.replace('graph_', '')
        matching_graphs = [graph for graph in graph_files if file_name == graph.replace('graph_', '').replace('.yaml', '')]
        matching_gurobis = [gurobi for gurobi in gurobi_files if file_name == gurobi.replace('gurobi_', '').replace('.yaml', '')]
        
        if len(matching_graphs) == 1 and len(matching_gurobis) == 1:
            match_dict[file_name] = {
                'graph': matching_graphs[0],
                'gurobi': matching_gurobis[0]
            }
            # print('file_name',file_name,'match_dict[file_name]',match_dict[file_name])
            # print(f'File Name: {file_name}, Matching Graphs: {matching_graphs}, Matching Gurobis: {matching_gurobis}')
        else:
            print(f'Match not found for {file_name}')
            # print(f'Match not found for {file_name},matching_graphs{matching_graphs},matching_gurobis{matching_gurobis}')
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
    # size_list = [(3,3),(3,5),(5,5)]
    size_list = [(4,15)]  # G5_T4_A15
    # size_list = [(t,a) for t in range(3,6) for a in range(5,25,5)]
    # size_list = [(t,15) for t in range(3,6)]
    print('size_list',size_list)

    for (max_task_num,max_agent_num) in size_list:
        max_group_num = 5
        base_name = f'G{max_group_num}_T{max_task_num}_A{max_agent_num}'
        gurobi_dir = f'data/{base_name}/gurobi_{base_name}'  # TODO how to deal with group=t1_t1
        graph_dir = f'data/{base_name}/graph_cbs_{base_name}'
        input_dir = f'data/{base_name}/random_inputs_{base_name}'
        save_dir = f'data/{base_name}/evaluation_{base_name}'

        match_dict = get_matched_files_dict(gurobi_dir,graph_dir,input_dir)
        match_cout = 0
        gurobi_runtime_list,graph_runtime_list =[],[]

        for matched_name,matched_files in match_dict.items():
            # print('matched_name',matched_name,'matched_files',matched_files)
            matching_gurobis = matched_files.get('gurobi')
            matching_graphs = matched_files.get('graph')
            if not matching_gurobis or not matching_graphs:
                print("Error: 'gurobi' or 'graph' key not found in dictionary.")
                continue

            res_gurobi,res_graph = parse_res(gurobi_dir,graph_dir,matching_gurobis,matching_graphs)
            # print('res_gurobi',res_gurobi)
            # print('res_graph',res_graph)

            if not os.path.exists(save_dir):
                os.makedirs(save_dir)
                
            if 'runtime' in res_gurobi and 'runtime' in res_graph:
                gurobi_runtime_list.append(res_gurobi['runtime'])
                graph_runtime_list.append(res_graph['runtime'])
            else:
                print('matched_name',matched_name)
                print('res_gurobi',res_gurobi)
                print('res_graph',res_graph)

            # gurobi_runtime_list.append(res_gurobi.get('runtime', None))
            # graph_runtime_list.append(res_graph.get('runtime', None))

            if res_gurobi['cost'] == res_graph['cost'] and res_gurobi['assignment'] == res_graph['assignment']:
                # print("Content of 'res_gurobi' and 'res_graph' is the same.")
                # print('!!!!!!')
                match_cout += 1
            else:
                print("Content of 'res_gurobi' and 'res_graph' is different.")

                # record the different input and also all the outputs            
                filename = f'{save_dir}/eval_{matched_name}.txt'
                with open(filename, 'w') as file:
                    file.write(f"res_gurobi:\n{res_gurobi}\n")
                    file.write(f"res_graph:\n{res_graph}\n")
                    with open(f'{input_dir}/{matched_name}.txt', 'r') as input_file:
                        file.write(f"content in {matched_name}.txt:\n{input_file.read()}\n")
        data = [gurobi_runtime_list, graph_runtime_list]


        print(" Creating violin plot")
        plt.close()
        fig, axs = plt.subplots(nrows=1, ncols=2, figsize=(15, 6))
        # fig = plt.figure(figsize=(8,6))
        axs[0].violinplot(data, showmeans=False,showmedians=True)
        axs[0].set_xticks([1, 2], ['Gurobi', 'Cbs'])
        axs[0].set_ylabel('Runtime')
        axs[0].set_title('Violin plot')

        axs[1].boxplot(data)
        axs[1].set_xticks([1, 2], ['Gurobi', 'Cbs'])
        axs[1].set_ylabel('Runtime')
        axs[1].set_title('Box plot')

        violin_path = save_dir + f'/violin_box_{base_name}.png'
        print('violin_path',violin_path)
        plt.savefig(violin_path, bbox_inches='tight')

        gurobi_runtime_length = len(gurobi_runtime_list)
        graph_runtime_length = len(graph_runtime_list)

        # print('gurobi_runtime_list',gurobi_runtime_list)
        # print('graph_runtime_list',graph_runtime_list)
        print("Length of gurobi_runtime_list:", gurobi_runtime_length)
        print("Length of graph_runtime_list:", graph_runtime_length)

        print('base_name',base_name)
        average_gurobi = sum(gurobi_runtime_list) / gurobi_runtime_length
        frequency_gurobi = 1/average_gurobi
        print(f'Average Runtime of !gurobi!: {average_gurobi} seconds')
        print(f'Frequency of !gurobi! Average Runtime: {frequency_gurobi}')


        average_cbs = sum(graph_runtime_list) / graph_runtime_length
        frequency_cbs = 1/average_cbs
        print(f'Average Runtime of !CBS!: {average_cbs} seconds')
        print(f'Frequency of !CBS! Average Runtime: {frequency_cbs}')

        print('match_cout',match_cout)
        print('end--------')








