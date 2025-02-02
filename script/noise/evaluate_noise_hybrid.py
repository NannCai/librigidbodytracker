import matplotlib.pyplot as plt
import numpy as np
# from sklearn.linear_model import LinearRegression
# from sklearn.preprocessing import PolynomialFeatures
import json

''' 
this script plots the correct count ratio vs total removed points (noise)
the workflow is first generate the noise data and estimated result 
via shell file add_point.sh and gen_noise_data_remove_point.sh
also cloudlog.hpp should be changed to add or remove points 
'''




# TODO add the data of old multi marker mode data, use another color to plot the points
    # TODO generate the state estimation result of old multi marker mode 
    # only change the mode in the c++ code and then run the same shell file (add_point.sh and gen_noise_data_remove_point.sh)

# evaluation of noise result
def parse_rigidbody_data(rigidbody_path):
    with open(rigidbody_path, 'r') as trans_file:
        rigidbody_data = trans_file.read().split(sep='\n')
    # print('Transformation Data:', rigidbody_data)

    in_transformation_block = False
    in_solution_block = False
    rb_trans_dict = {}
    for line in rigidbody_data[:]:
        if line.startswith('stamp:'):
            key = line
            rb_trans_dict[key] = {"trans":{},"solution":{}}	
            in_transformation_block = False
        elif line.startswith('solution:'):
            in_solution_block = True
        elif line.startswith('transformation:'):
            in_transformation_block = True
            in_solution_block = False
        elif in_solution_block:
            if line:
                # print("line",line)
                index, values = line.split(': ', 1)
                rb_trans_dict[key]["solution"][int(index)] =np.array(list(map(int, values.split())))  #list(map(int, values.split()))

        elif in_transformation_block:
            # print("line",line)
            if line:
                index, values = line.split(': ', 1)
                # rb_trans_dict[key]["trans"][int(index)] = list(map(float, values.split()))  
                rb_trans_dict[key]["trans"][int(index)] =np.array(list(map(lambda x: round(float(x), 4), values.split())))  #list(map(lambda x: round(float(x), 4), values.split()))

    # print('rb_trans_dict',rb_trans_dict)
    return rb_trans_dict
    
if __name__ == '__main__':
    # root_path = 'data/output_remove'
    # root_path = 'data/output_add_point'
    # root_path = 'data/output_test_remove_add'
    # root_path = './data/output_add_remove_1208'
    root_path = './data/output_add_remove_hybrid_mode_2d_7m_mo_0701'

    noise_info_file_name  = 'noise_info.txt'
    noise_info_path = f'{root_path}/{noise_info_file_name}'


    with open(noise_info_path, 'r') as noise_info_file:
        noise_info  = noise_info_file.read().split(sep='\n')
    # print('noise_info',noise_info)

    noise_res_info_dict = {}
    ground_truth_name = 0
    for item in noise_info:
        if item.endswith(':'):
            key = item[:-1]
            noise_res_info_dict[key] = []
        elif "pick_probability" in item:
            # noise_res_info_dict[key].append(item[:-1])
            key_value = item.split(': ')
            if len(key_value) < 2:
                print('len(key_value) < 2',key_value)
                continue
            noise_res_info_dict[key].append([key_value[0], float(key_value[1])]) 
        elif "total_removed_points" in item:
            key_value = item.split(': ')
            if len(key_value) < 2:
                print('len(key_value) < 2',key_value)
                continue
            noise_res_info_dict[key].append([key_value[0], int(key_value[1])]) 
            if int(key_value[1]) == 0:
                ground_truth_name = key
                print("Key with value 0:", ground_truth_name)    

    # print('noise_res_info_dict',noise_res_info_dict)
    # quit()

    if ground_truth_name == 0:
        print('!!cannot find the ground_truth point cloud!!')
        quit()
    # parse gt transformations
    # gt_path = f'{root_path}/{ground_truth_name}.txt'
    gt_path = f'{ground_truth_name}.txt'
    gt_trans_dict = parse_rigidbody_data(gt_path)
    # print('gt_trans_dict',gt_trans_dict)

    # th = [0.0001 for _ in range(6)]

    result_dict = {}

    for file_name, file_info in noise_res_info_dict.items():
        noise_path =  f'{file_name}.txt'
        noise_trans_dict = parse_rigidbody_data(noise_path)
        correct_count = 0
        incorrect_count = 0

        for rb_key, gt_value in gt_trans_dict.items():
            gt_trans = gt_value.get("trans")
            noise_trans = noise_trans_dict.get(rb_key, {}).get("trans", {})
            

            for rb, gt_t in gt_trans.items():
                noise_t = noise_trans.get(rb)
                # if (P.solution.empty()) nothing saved maybe also saved the stamp and empty solution
                if noise_t is None:  # TODO each time is like skip one frame,   noise_trans {}
                    # print('----------')
                    # print('noise_path',noise_path)
                    # print('rb_key',rb_key)
                    # print('noise_trans',noise_trans)
                    # print('gt_trans',gt_trans)
                    # print('noise_t is None,rb',rb)
                    incorrect_count += 1
                    continue

                noise_solution_keys = noise_trans_dict[rb_key]['solution'].keys()
                if rb not in noise_solution_keys:
                    # print('rb not in noise_solution_keys',rb,noise_solution_keys)
                    incorrect_count += 1
                    continue                    
                
                # difference = [abs(gt - noise) for gt, noise in zip(gt_t, noise_t)]
                # if all(diff >= 0.0005 for diff in difference):
                difference = np.abs(gt_t - noise_t)
                # TODO  
                # import rowan.geometry.sym_intrinsic_distance  
                # only both rotation and translation are close enough, then it is correct
                
                np.linalg.norm(gt_t -noise_t)  # use euclidean distance to compare the difference
                if np.all(difference >= 0.005):  # TODO too small , change to 0.005
                    incorrect_count += 1
                    # print(f"Difference: {difference}")
                    # print(f"Noise transformation is incorrect for rb: {rb}")
                    # print(f"GT Trans: {gt_trans}")
                    # print(f"Noise Trans: {noise_trans}")
                else:
                    correct_count += 1
        result_dict[file_name] = {}
        result_dict[file_name]["count"] = [correct_count, incorrect_count]
        result_dict[file_name]['info'] = file_info
        x = file_info[0][1]
        y = correct_count/(correct_count + incorrect_count)
        result_dict[file_name]['plot'] = [x,y]


    # print('Noise Dictionary:', noise_res_info_dict)
    print('Result Dictionary:', result_dict)

    # Save result_dict to a JSON file
    with open('./script/noise/result_dict_hybrid.json', 'w') as f:
        json.dump(result_dict, f, indent=4)

    total_list = [data['count'][0] + data['count'][1] for data in result_dict.values()]
    print('Total Count:', total_list)

    # Extracting x and y values from result_dict
    x_values = []
    y_values = []

    for file_name, data in result_dict.items():
        plot_data = data['plot']
        x_values.append(plot_data[0])
        y_values.append(plot_data[1])

    # print('x_values',x_values)
    # print('y_values',y_values)

    # only points plot
    plt.figure(figsize=(10, 6))
    plt.scatter(x_values, y_values, color='blue')

    # Adding labels and title
    plt.xlabel('Total Removed Points')
    plt.ylabel('Correct Count Ratio')
    plt.title('Total Removed Points vs Correct Count Ratio')

    # Show the plot
    plt.grid(True)
    # plt.show() 
    save_path = f'./script/noise/evaluate_noise_{root_path.split("/")[-1]}.png'
    plt.savefig(save_path, bbox_inches='tight')
    print('Save Path:',save_path)


    # # linear fit
    # x_values = np.array(x_values).reshape(-1, 1)  # Reshape for sklearn
    # y_values = np.array(y_values)

    # # Creating and fitting the linear regression model
    # model = LinearRegression()
    # model.fit(x_values, y_values)

    # # Predicting y values for the fitted line
    # y_pred = model.predict(x_values)

    # # Creating the plot
    # plt.figure(figsize=(10, 6))
    # plt.scatter(x_values, y_values, color='blue', label='Data points')
    # plt.plot(x_values, y_pred, color='red', label='Fitted line')

    # # Adding labels and title
    # plt.xlabel('Total Removed Points')
    # plt.ylabel('Correct Count Ratio')
    # plt.title('Total Removed Points vs Correct Count Ratio')
    # plt.legend()

    # # Show the plot
    # plt.grid(True)
    # plt.show()




    # # polynomial fit
    # x_values = np.array(x_values).reshape(-1, 1)  # Reshape for sklearn
    # y_values = np.array(y_values)

    # # Degree of the polynomial
    # degree = 2  # You can change the degree to fit more complex curves

    # # Creating polynomial features
    # poly_features = PolynomialFeatures(degree=degree)
    # x_poly = poly_features.fit_transform(x_values)

    # # Creating and fitting the polynomial regression model
    # model = LinearRegression()
    # model.fit(x_poly, y_values)

    # # Predicting y values for the fitted polynomial curve
    # y_poly_pred = model.predict(x_poly)

    # # Creating the plot
    # plt.figure(figsize=(10, 6))
    # plt.scatter(x_values, y_values, color='blue', label='Data points')
    # plt.plot(x_values, y_poly_pred, color='red', label=f'Polynomial degree {degree}')

    # # Adding labels and title
    # plt.xlabel('Total Removed Points')
    # plt.ylabel('Correct Count Ratio')
    # plt.title('Total Removed Points vs Correct Count Ratio')
    # plt.legend()

    # # Show the plot
    # plt.grid(True)
    # plt.show()













