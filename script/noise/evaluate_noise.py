

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
    key = ''
    ground_truth_name = 0
    for item in noise_info:
        if item.endswith(':'):
            key = item[:-1]
        else:
            key_value = item.split(': ')
            if len(key_value) < 2:
                print('len(key_value) < 2',key_value)
                continue
            noise_dict[key] = [key_value[0], int(key_value[1])]
            if int(key_value[1]) == 0:
                ground_truth_name = key
                print("Key with value 0:", ground_truth_name)    

    # print('noise_dict',noise_dict)

    if ground_truth_name == 0:
        print('!!cannot find the ground_truth point cloud!!')
        quit()
    # parse gt transformations
    gt_path = f'{root_path}/{ground_truth_name}.txt'
    gt_trans_dict = parse_rigidbody_data(gt_path)
    # print('gt_trans_dict',gt_trans_dict)

    th = [0.0001 for _ in range(6)]


    result_dict = {}

    for key,value in noise_dict.items():
        noise_path = f'{root_path}/{key}.txt'
        noise_trans_dict  = parse_rigidbody_data(noise_path)
        correct_count = 0
        incorrect_count = 0
        # print('noise_trans_dict',noise_trans_dict)
        for k,v in gt_trans_dict.items():
            gt_trans = gt_trans_dict.get(k, {}).get("trans")
            noise_trans = noise_trans_dict.get(k, {}).get("trans")
            # print('gt_trans',gt_trans)
            # print('noise_trans',noise_trans)
            for rb, gt_t in gt_trans.items():
                print
                noise_t = noise_trans.get(rb, None)
                if noise_t is None:
                    print('noise_t is None',noise_t)
                    continue
                difference = [abs(gt - noise) for gt, noise in zip(gt_t, noise_t)]
                if all(diff >= 0.0005 for diff in difference):
                    incorrect_count += 1
                    print('difference', difference)
                    print("----Noise transformation is incorrect for rb:---", rb)
                    print('gt_trans', gt_trans)
                    print('noise_trans', noise_trans)
                else:
                    correct_count += 1
        # print("Number of correct transformations:", correct_count)
        # print("Number of incorrect transformations:", incorrect_count)

        result_dict[key] = [correct_count, incorrect_count]

    print('noise_dict',noise_dict)
    print('result_dict',result_dict)



















