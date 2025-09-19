import os

def combine_platformio_files(root_folder, output_file):
    """
    Combines all .h, .cpp, and platformio.ini files from root_folder into a single text file.
    
    Args:
        root_folder (str): Path to the root directory containing PlatformIO files
        output_file (str): Path to the output text file
    """

    with open(output_file, 'w', encoding='utf-8') as outfile:
        for dirpath, dirs, filenames in os.walk(root_folder):
            # Exclude specific directories
            exclude_dirs = {'.git', '.pio', '.vscode'}
            dirs[:] = [d for d in dirs if d not in exclude_dirs]

            for filename in filenames:
                # Skip all hidden files (e.g. .gitignore, .env, .DS_Store, etc.)
                if filename.startswith('.'):
                    continue

                # Process only .h, .cpp, or platformio.ini
                if filename.endswith(('.h', '.cpp')) or filename == 'platformio.ini':
                    file_path = os.path.join(dirpath, filename)
                    print(file_path)

                    outfile.write(f"\n\n{'='*60}\n")
                    outfile.write(f"File: {file_path}\n")
                    outfile.write(f"{'='*60}\n\n")

                    try:
                        with open(file_path, 'r', encoding='utf-8') as infile:
                            outfile.write(infile.read())
                    except Exception as e:
                        outfile.write(f"Error reading file: {str(e)}\n")
    

if __name__ == "__main__":
    # Configuration - modify these paths as needed
    PLATFORMIO_ROOT = "."  # Current directory (change to your PlatformIO project path)
    OUTPUT_FILENAME = "platformio_combined.txt"
    
    combine_platformio_files(PLATFORMIO_ROOT, OUTPUT_FILENAME)
    print(f"Combined PlatformIO files into {OUTPUT_FILENAME}")