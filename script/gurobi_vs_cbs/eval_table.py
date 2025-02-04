import glob 
import os
import yaml

def frequency_table():

    for key, value in conclusion_dict.items():
        # avg_gurobi_runtime = value.get('average_gurobi_runtime', 'N/A')
        # avg_cbs_runtime = value.get('average_cbs_runtime', 'N/A')  # frequency_gurobi_runtime
        frequency_gurobi_runtime = value.get('frequency_gurobi_runtime', 'None')
        frequency_cbs_runtime = value.get('frequency_cbs_runtime', 'None')  

        if frequency_gurobi_runtime > frequency_cbs_runtime:
            frequency_gurobi_runtime = f"\\textbf{{{frequency_gurobi_runtime:.1f}}}"
            frequency_cbs_runtime = f"{frequency_cbs_runtime:.1f}"
        elif frequency_cbs_runtime>frequency_gurobi_runtime:
            frequency_gurobi_runtime = f"{frequency_gurobi_runtime:.1f}"
            frequency_cbs_runtime = f"\\textbf{{{frequency_cbs_runtime:.1f}}}"
        else:
            print('!!wrong!!')
            
        key = key.replace('_', r'\_')
        print(f"{key} & {frequency_gurobi_runtime} & {frequency_cbs_runtime} \\\\")

def avg_table():

    for key, value in conclusion_dict.items():
        # Retrieve average runtimes and convert to milliseconds
        avg_gurobi = value.get('average_gurobi_runtime', 0.0) * 1000  # Convert to ms
        avg_cbs = value.get('average_cbs_runtime', 0.0) * 1000        # Convert to ms



        # Determine which average is larger and format with LaTeX bold
        if avg_gurobi < avg_cbs:
            avg_gurobi_str = f"\\textbf{{{avg_gurobi:.3f}}}"
            avg_cbs_str = f"{avg_cbs:.3f} "
        elif avg_cbs < avg_gurobi:
            avg_gurobi_str = f"{avg_gurobi:.3f} "
            avg_cbs_str = f"\\textbf{{{avg_cbs:.3f}}}"
        else:
            # If equal, no bold (optional: handle as needed)
            avg_gurobi_str = f"{avg_gurobi:.3f} "
            avg_cbs_str = f"{avg_cbs:.3f} "


        # # key = key.replace('_', r'\_')  # Escape underscores for LaTeX
        # key = key.replace('_', r'&')  # Escape underscores for LaTeX
        # print(f"{key} & {avg_gurobi_str} & {avg_cbs_str} \\\\")



        parts = key.split('_')  # Split key into components (e.g., ['G3', 'T3', 'A5'])
        modified_parts = [part[1:] for part in parts]  # Remove first character from each part
        formatted_key = '&'.join(modified_parts)  # Join with '&' for LaTeX table
        print(f"{formatted_key} & {avg_gurobi_str} & {avg_cbs_str} \\\\")






if __name__ == '__main__':


    random_data_root_dir = 'data/gurobi_vs_cbs2'
    conclusion_files = glob.glob(f'{random_data_root_dir}/evaluation_conclusion*')
    # for file in conclusion_files:
    #     print(file)  # Print or process the files as needed

    if len(conclusion_files) != 1:
        print(len(conclusion_files) != 1)
        print('conclusion_files',conclusion_files)
        quit()
    conclusion_file = conclusion_files[0] 


    if os.path.exists(conclusion_file):
        try:
            with open(conclusion_file, 'r') as file:
                conclusion_dict = yaml.safe_load(file)
                # If you expect the file to be a specific type of YAML structure, you might want to validate it here
        except yaml.YAMLError as e:
            print(f"Error parsing YAML file: {e}")
        except IOError as e:
            print(f"Error opening file: {e}")
    else:
        print("File does not exist.")
    
    # print('conclusion_dict',conclusion_dict)

    # frequency_table()
    avg_table()


