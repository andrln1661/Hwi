import os
import argparse
from pathlib import Path

def combine_platformio_files(root_dir, output_file, exclude_dirs=None, exclude_extensions=None):
    """
    Combines common PlatformIO project files into a single text file with clear separation.
    
    Args:
        root_dir (str): Root directory to search for files
        output_file (str): Output text file path
        exclude_dirs (list): List of directory names to exclude
        exclude_extensions (list): List of file extensions to exclude
    """
    if exclude_dirs is None:
        exclude_dirs = ['.git', '.svn', '.pio', '.vscode', 'lib_deps']
    
    if exclude_extensions is None:
        exclude_extensions = ['.png', '.jpg', '.jpeg', '.gif', '.ico', '.bin', '.hex', '.elf', '.o', '.a', '.so', '.dll', '.exe']
    
    # Common PlatformIO file extensions and names to include
    include_extensions = [
        '.py', '.ini', '.txt', '.md', '.yaml', '.yml', '.json', '.xml', 
        '.cpp', '.c', '.h', '.hpp', '.ino', '.S', '.ld', '.cfg'
    ]
    
    include_names = [
        'platformio.ini', 'README', 'README.md', 'LICENSE', 'library.json', 
        'library.properties', 'keywords.txt', 'platform.txt', 'boards.txt'
    ]
    
    with open(output_file, 'w', encoding='utf-8') as outfile:
        outfile.write("# PlatformIO Project Files\n\n")
        
        # Use rglob to recursively find all files
        for file_path in Path(root_dir).rglob("*"):
            # Skip if it's a directory
            if file_path.is_dir():
                continue
                
            # Skip excluded directories
            if any(excluded in file_path.parts for excluded in exclude_dirs):
                continue
                
            # Skip excluded extensions
            if file_path.suffix.lower() in exclude_extensions:
                continue
                
            # Include files with specific extensions or names
            if (file_path.suffix.lower() in include_extensions or 
                file_path.name in include_names or
                file_path.name.lower().startswith('readme')):
                
                relative_path = file_path.relative_to(root_dir)
                try:
                    with open(file_path, 'r', encoding='utf-8') as infile:
                        content = infile.read()
                        
                    # Write clear file separation
                    outfile.write(f"\n{'='*80}\n")
                    outfile.write(f"File: {relative_path}\n")
                    outfile.write(f"{'='*80}\n\n")
                    outfile.write(content)
                    outfile.write("\n")
                    
                except UnicodeDecodeError:
                    # Handle binary files that might have slipped through
                    outfile.write(f"\n{'='*80}\n")
                    outfile.write(f"File: {relative_path}\n")
                    outfile.write(f"{'='*80}\n")
                    outfile.write("[Binary file - content not included]\n\n")
                except Exception as e:
                    print(f"Warning: Could not read {relative_path}: {e}")

def main():
    parser = argparse.ArgumentParser(description="Combine PlatformIO project files into a single text file")
    parser.add_argument("project_dir", help="PlatformIO project directory path")
    parser.add_argument("-o", "--output", default="platformio_project.txt", 
                        help="Output file name (default: platformio_project.txt)")
    parser.add_argument("-e", "--exclude", nargs='*', default=None,
                        help="Additional directories to exclude")
    parser.add_argument("--exclude-ext", nargs='*', default=None,
                        help="Additional file extensions to exclude")
    
    args = parser.parse_args()
    
    # Prepare exclusion list
    exclude_dirs = ['.git', '.pio', '.vscode', '.svn']
    if args.exclude:
        exclude_dirs.extend(args.exclude)
        
    exclude_ext = ['.png', '.jpg', '.jpeg', '.gif', '.ico', '.bin', '.hex']
    if args.exclude_ext:
        exclude_ext.extend(args.exclude_ext)
    
    # Verify project directory exists
    if not os.path.isdir(args.project_dir):
        print(f"Error: Directory '{args.project_dir}' does not exist")
        return
    
    # Combine files
    combine_platformio_files(args.project_dir, args.output, exclude_dirs, exclude_ext)
    print(f"Combined PlatformIO files saved to: {args.output}")

if __name__ == "__main__":
    main()