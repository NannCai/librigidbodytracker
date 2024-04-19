import os
import shutil

# Source and destination directories
src_dir = 'data/18_04/'
dst_dir = 'data/19_04/'

# List all directories in the source directory
for dirpath, dirnames, filenames in os.walk(src_dir):
    for dirname in dirnames:
        # Check if the directory name starts with 'random_inputs_'
        if dirname.startswith('random_inputs_'):
            # Construct the source and destination paths
            src_path = os.path.join(dirpath, dirname)
            dst_path = src_path.replace(src_dir, dst_dir, 1)
            # Create the destination directory if it doesn't exist
            os.makedirs(dst_path, exist_ok=True)
            # Copy all files from the source directory to the destination directory
            for filename in os.listdir(src_path):
                shutil.copy(os.path.join(src_path, filename), dst_path)