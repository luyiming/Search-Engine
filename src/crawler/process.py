import os

max_num = 0
s=''

def generate_edges(path):
    global max_num
    global s
    if not os.path.isdir(path) and not os.path.isfile(path):
        return False
    if os.path.isfile(path):
        try:
            num = int(os.path.basename(path))
            if num > max_num:
                max_num = num
                s = path
        except:
            pass
    if os.path.isdir(path):
        for x in os.listdir(path):
            generate_edges(os.path.join(path, x))

generate_edges('../sites')
print(max_num)
print(s)
