import matplotlib.pyplot as plt
import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.preprocessing import PolynomialFeatures


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
                rb_trans_dict[key]["solution"][int(index)] = list(map(int, values.split()))

        elif in_transformation_block:
            # print("line",line)
            if line:
                index, values = line.split(': ', 1)
                # rb_trans_dict[key]["trans"][int(index)] = list(map(float, values.split()))  
                rb_trans_dict[key]["trans"][int(index)] = list(map(lambda x: round(float(x), 4), values.split()))

    # print('rb_trans_dict',rb_trans_dict)
    return rb_trans_dict
    
if __name__ == '__main__':
    root_path = 'data/output'

    noise_info_file_name  = 'noise_info.txt'
    noise_info_path = f'{root_path}/{noise_info_file_name}'


    with open(noise_info_path, 'r') as noise_info_file:
        noise_info  = noise_info_file.read().split(sep='\n')
    # print('noise_info',noise_info)

    noise_dict = {}
    # key = ''
    ground_truth_name = 0
    for item in noise_info:
        if item.endswith(':'):
            key = item[:-1]
            noise_dict[key] = []
        elif "pick_probability" in item:
            # noise_dict[key].append(item[:-1])
            key_value = item.split(': ')
            if len(key_value) < 2:
                print('len(key_value) < 2',key_value)
                continue
            noise_dict[key].append([key_value[0], float(key_value[1])]) 
        elif "total_removed_points" in item:
            key_value = item.split(': ')
            if len(key_value) < 2:
                print('len(key_value) < 2',key_value)
                continue
            noise_dict[key].append([key_value[0], int(key_value[1])]) 
            if int(key_value[1]) == 0:
                ground_truth_name = key
                print("Key with value 0:", ground_truth_name)    

    # print('noise_dict',noise_dict)
    # quit()

    if ground_truth_name == 0:
        print('!!cannot find the ground_truth point cloud!!')
        quit()
    # parse gt transformations
    # gt_path = f'{root_path}/{ground_truth_name}.txt'
    gt_path = f'{ground_truth_name}.txt'
    gt_trans_dict = parse_rigidbody_data(gt_path)
    # print('gt_trans_dict',gt_trans_dict)

    th = [0.0001 for _ in range(6)]


    result_dict = {}

    for file_name, file_info in noise_dict.items():
        # noise_path = f'{root_path}/{file_name}.txt'
        noise_path =  f'{file_name}.txt'
        noise_trans_dict = parse_rigidbody_data(noise_path)
        correct_count = 0
        incorrect_count = 0

        for rb_key, gt_value in gt_trans_dict.items():
            gt_trans = gt_value.get("trans")
            noise_trans = noise_trans_dict.get(rb_key, {}).get("trans", {})
            
            for rb, gt_t in gt_trans.items():
                noise_t = noise_trans.get(rb)
                
                if noise_t is None:
                    print('noise_t is None',noise_t)
                    continue
                
                difference = [abs(gt - noise) for gt, noise in zip(gt_t, noise_t)]
                if all(diff >= 0.0005 for diff in difference):
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


    print('Noise Dictionary:', noise_dict)
    print('Result Dictionary:', result_dict)


    # Extracting x and y values from result_dict
    x_values = []
    y_values = []

    for file_name, data in result_dict.items():
        plot_data = data['plot']
        x_values.append(plot_data[0])
        y_values.append(plot_data[1])

    print('x_values',x_values)
    print('y_values',y_values)

    # only points plot
    plt.figure(figsize=(10, 6))
    plt.scatter(x_values, y_values, color='blue')

    # Adding labels and title
    plt.xlabel('Total Removed Points')
    plt.ylabel('Correct Count Ratio')
    plt.title('Total Removed Points vs Correct Count Ratio')

    # Show the plot
    plt.grid(True)
    plt.show()


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













