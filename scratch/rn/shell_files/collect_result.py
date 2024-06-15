import os
import subprocess

def execute_command_in_all_folders(cmd_command):
    for root, dirs, files in os.walk('.'):
        for directory in dirs:
            folder_path = os.path.join(root, directory)
            os.chdir(folder_path)  # 进入文件夹
            try:
                subprocess.run(cmd_command, shell=True, check=True)
            except subprocess.CalledProcessError as e:
                print(f"命令执行失败")
            os.chdir('..')  # 返回上一级目录

# 在所有文件夹下执行指定命令
cmd_command = "cp ../parse-results.py ./"
execute_command_in_all_folders(cmd_command)
cmd_command = "python parse-results.py flow.xml"
execute_command_in_all_folders(cmd_command)


def merge_txt_files(folder_path, target_filename):
    with open(target_filename, 'w') as target_file:
        for root, dirs, files in os.walk(folder_path):
            for file in files:
                if file.endswith(".txt") and file == "result.txt":
                    file_path = os.path.join(root, file)
                    with open(file_path, 'r') as source_file:
                        target_file.write(source_file.read())

# 在当前目录下合并所有文件夹中的 example.txt 文件
merge_txt_files(".", "merged_result.txt")
