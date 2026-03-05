import matplotlib.pyplot as plt

def plot_servers(sId):
    file_name = "server_" + str(sId)
    with open(file_name) as f:
        print("bla")
    f.close()

def merge_runs(num_runs, m): # might be a good idea to take f as an argument instead of opening and closing it multiple times
                             # but will do this later after checking where to call this function (can create a file and then pass the file pointer)
    consolidated_file_name = "consolidated_client_data.csv"
    open_files = [""] * num_runs

    for i in range(num_runs):
        file_name = "main_" + str(i + 1)
        open_files[i] = open(file_name)
    
    with open(consolidated_file_name, "w") as f:
        for i in range(m):
            for j in range(num_runs):
                line = open_files[j].readline()
                if line:
                    f.write(line)
    f.close()

    with open(consolidated_file_name, "w") as f:
        for i in range(num_runs):
            file_name = "main_" + str(i)
            with open(file_name) as f1:
                for line in f1:
                    f.write(line)
            f1.close()
    f.close()

# def merge_sys_data(num_servers):
#     consolidated_file_name = "consolidated_sys_data.csv"
#     with open(consolidated_file_name, "w") as f:
#         for i in range(num_servers):
#             file_name = "server_" + str(i+1)
#             with open(file_name) as f1:
#                 for line in f1:
#                     f.write(line)
#             f1.close()
#     f.close()