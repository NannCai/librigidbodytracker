#!/usr/bin/env python3
import argparse
import json
import logging
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def parse_rigidbody_data(file_path: Path) -> dict:
    """
    Parse the rigidbody transformation file.
    
    The file is expected to be in the following format:
        stamp: <timestamp>
        transformation:
            <index>: <value value value ...>
            ...
        solution:
            <index>: <value value value ...>
            ...
    
    Returns a dictionary keyed by the stamp with sub-dictionaries for 'trans' and 'solution'.
    """
    logging.info("Parsing rigidbody data from: %s", file_path)
    with file_path.open('r') as trans_file:
        lines = trans_file.read().splitlines()

    rb_dict = {}
    current_stamp = None
    in_transformation_block = False
    in_solution_block = False

    for line in lines:
        if line.startswith('stamp:'):
            current_stamp = line.strip()
            rb_dict[current_stamp] = {"trans": {}, "solution": {}}
            in_transformation_block = False
            in_solution_block = False
        elif line.startswith('transformation:'):
            in_transformation_block = True
            in_solution_block = False
        elif line.startswith('solution:'):
            in_solution_block = True
            in_transformation_block = False
        elif line.strip():
            try:
                index_str, values_str = line.split(': ', 1)
                index = int(index_str.strip())
                values = values_str.split()
                if in_transformation_block:
                    # Round values to 4 decimal places and convert to float
                    rb_dict[current_stamp]["trans"][index] = np.array(
                        [round(float(v), 4) for v in values]
                    )
                elif in_solution_block:
                    rb_dict[current_stamp]["solution"][index] = np.array(
                        list(map(int, values))
                    )
            except Exception as e:
                logging.error("Error parsing line '%s': %s", line, e)
    return rb_dict


def load_noise_info(noise_info_path: Path) -> (dict, str):
    """
    Loads the noise information from a file and returns a dictionary containing noise details.
    
    The file is assumed to contain blocks starting with a key ending with ':'
    and then lines with key-value pairs, e.g.:
        some_key:
        pick_probability: 1.0
        total_removed_points: 0

    Returns a tuple:
        - noise_info: A dict mapping file keys to a list of [key, value] pairs.
        - ground_truth_name: The file key for which total_removed_points is 0.
    """
    logging.info("Loading noise info from: %s", noise_info_path)
    with noise_info_path.open('r') as f:
        lines = f.read().splitlines()

    noise_info = {}
    ground_truth_name = None
    current_key = None

    for line in lines:
        if line.endswith(':'):
            current_key = line[:-1]
            noise_info[current_key] = []
        elif "pick_probability" in line or "total_removed_points" in line:
            try:
                key_val = line.split(': ', 1)
                if len(key_val) < 2:
                    logging.warning("Skipping line with unexpected format: %s", line)
                    continue
                key, value = key_val[0].strip(), key_val[1].strip()
                if key == "pick_probability":
                    noise_info[current_key].append([key, float(value)])
                elif key == "total_removed_points":
                    points = int(value)
                    noise_info[current_key].append([key, points])
                    if points == 0:
                        ground_truth_name = current_key
                        logging.info("Found ground truth with key: %s", ground_truth_name)
            except Exception as e:
                logging.error("Error processing line '%s': %s", line, e)
    if ground_truth_name is None:
        logging.error("No ground truth point cloud found (total_removed_points == 0)")
        raise ValueError("Ground truth not found in noise info.")
    return noise_info, ground_truth_name


def evaluate_results(gt_trans_dict: dict, noise_trans_dict: dict) -> (int, int):
    """
    Compare the ground truth and noise transformations.
    
    Returns the count of correct and incorrect transformation estimates.
    A transformation is considered correct if:
        - The transformation exists in the noise file.
        - (Optionally) For files with a 'solution' block, the corresponding key exists.
        - The difference (L2 norm) between ground truth and noise transformation is below 0.0005.
    """
    correct_count = 0
    incorrect_count = 0

    for stamp, gt_data in gt_trans_dict.items():
        gt_trans = gt_data.get("trans", {})
        noise_data = noise_trans_dict.get(stamp, {})
        noise_trans = noise_data.get("trans", {})

        noise_solution = noise_data.get("solution")
        for idx, gt_val in gt_trans.items():
            noise_val = noise_trans.get(idx)
            if noise_val is None:
                incorrect_count += 1
                continue

            if noise_solution is not None:
                # Ensure the noise solution has the corresponding index
                if idx not in noise_solution:
                    incorrect_count += 1
                    continue

            # Use L2 norm difference
            difference = np.linalg.norm(gt_val - noise_val)
            if difference >= 0.0005:
                incorrect_count += 1
            else:
                correct_count += 1

    return correct_count, incorrect_count


def plot_results(result_dict: dict, output_path: Path):
    """
    Generate and save a scatter plot of total removed points vs correct count ratio.
    
    The result_dict is expected to have file keys with a 'plot' entry [x, y].
    """
    x_values = []
    y_values = []
    for data in result_dict.values():
        x, y = data.get('plot', [None, None])
        if x is not None and y is not None:
            x_values.append(x)
            y_values.append(y)

    plt.figure(figsize=(10, 6))
    plt.scatter(x_values, y_values, color='blue')
    plt.xlabel('Total Removed Points')
    plt.ylabel('Correct Count Ratio')
    plt.title('Total Removed Points vs Correct Count Ratio')
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(output_path)
    logging.info("Plot saved to: %s", output_path)


def main(args):
    # Setup logging
    logging.basicConfig(level=logging.INFO, format='[%(levelname)s] %(message)s')

    root_path = Path(args.root_dir)
    noise_info_path = root_path / 'noise_info.txt'

    try:
        noise_info, ground_truth_key = load_noise_info(noise_info_path)
    except ValueError as ve:
        logging.error("Exiting due to error: %s", ve)
        return

    # Assume ground truth file is in the current working directory or adjust as needed.
    gt_file_path = Path(f"{ground_truth_key}.txt")
    gt_trans_dict = parse_rigidbody_data(gt_file_path)

    result_dict = {}

    # Process each noise file
    for file_key, info in noise_info.items():
        noise_file_path = Path(f"{file_key}.txt")
        if not noise_file_path.exists():
            logging.warning("File %s does not exist, skipping.", noise_file_path)
            continue

        noise_trans_dict = parse_rigidbody_data(noise_file_path) 
        correct_count, incorrect_count = evaluate_results(gt_trans_dict, noise_trans_dict)

        # Extract the 'total_removed_points' from the info list.
        # We assume the list contains an entry like ['total_removed_points', value].
        total_removed_points = None
        for item in info:
            if item[0] == 'total_removed_points':
                total_removed_points = item[1]
                break
        if total_removed_points is None:
            logging.warning("total_removed_points not found for %s; skipping.", file_key)
            continue

        # Calculate correct count ratio
        total = correct_count + incorrect_count
        ratio = correct_count / total if total > 0 else 0

        result_dict[file_key] = {
            "count": [correct_count, incorrect_count],
            "info": info,
            "plot": [total_removed_points, ratio]
        }
        logging.info("Processed %s: correct=%d, incorrect=%d, ratio=%.3f",
                     file_key, correct_count, incorrect_count, ratio)

    # Save result dictionary as JSON
    # Choose filename based on whether a noise 'solution' exists in one of the files
    noise_has_solution = any('solution' in data and data['solution'] for data in result_dict.values())
    result_json_name = "result_dict_hybrid.json" if noise_has_solution else "result_dict_old_multi.json"
    result_json_path = Path('./script/noise') / result_json_name
    result_json_path.parent.mkdir(parents=True, exist_ok=True)
    with result_json_path.open('w') as f:
        json.dump(result_dict, f, indent=4)
    logging.info("Results saved to JSON: %s", result_json_path)

    # Plot the results
    plot_output_path = Path('./script/noise') / f'evaluate_noise_{root_path.name}.png'
    plot_results(result_dict, plot_output_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Evaluate noise results by comparing rigidbody transformations and plotting statistics."
    )
    parser.add_argument(
        "--root_dir",
        type=str,
        default="./data/output_add_remove_old_multi_mode_2d_7m_mo_0701",
        help="Path to the root directory containing noise_info.txt and noise files."
    )
    args = parser.parse_args()
    main(args)
