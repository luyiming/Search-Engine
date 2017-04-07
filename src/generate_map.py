'''
author:  Lu Yiming <luyimingchn@gmail.com>
version: 1.0
date: 2017-1-4

1. generate document ID
2. generate vertex file: graph.txt
3. generate map file: map.txt
4. moving files in sites/ to doc/ for better manipulation
5. generate doc/_.map which specifies existing documents

P.S. the document-ID is generated for all urls in graph, however parts of them
     don't have corresponding document files
'''
import os
import shutil

count = 0
docFileCount = 0
map_url_id = {}

graphFilePath = '../data/graph.txt'
mapFilePath = '../data/map.txt'
sitesDataPath = '../sites'
docPath = '../data/doc'
docIDFilePath = '../data/doc/_.map'


def get_id(url, title, path):
    global count
    global docFileCount
    res_id = map_url_id.get(url)
    if res_id is None:
        res_id = count
        map_url_id[url] = count
        count = count + 1
    if title is None:
        title = 'NULL'
        path = 'NULL'
    else:
        path = os.path.abspath(path)
        shutil.copyfile(path, os.path.join(docPath, str(res_id)))
        docFileCount = docFileCount + 1
        with open(docIDFilePath, 'a') as f:
            f.write("%d %s\n%s\n" % (res_id, url, title))
    with open(mapFilePath, 'a') as f:
        f.write("%d\n%s\n%s\n%s\n" % (res_id, url, title, path))
    return res_id

def generate(path):
    if not os.path.isdir(path) and not os.path.isfile(path):
        return False
    if os.path.isfile(path):
        if os.path.splitext(path)[-1] == '.graph':
            textFilePath = os.path.splitext(path)[0]
            if not os.path.exists(textFilePath):
                print("not match %s" % textFilePath)
                return False
            with open(path, "r") as f:
                title = f.readline().strip()
                url = f.readline().strip()
                src_id = get_id(url, title, textFilePath)
                with open(graphFilePath, 'a') as edge_file:
                    for line in f.readlines():
                        dst_id = get_id(line.strip(), None, None)
                        edge_file.write('%d %d\n' % (src_id, dst_id))
    elif os.path.isdir(path):
        for x in os.listdir(path):
            generate(os.path.join(path, x))

def init():
    with open(mapFilePath, 'w') as f:
        f.write("index\nurl\ntitle\npath\n")
    with open(graphFilePath, 'w') as f:
        f.write('# src dest\n')
    if not os.path.exists(docPath):
        os.mkdir(docPath)
    with open(docIDFilePath, 'w') as f:
        f.write('')


print("generating edges/map files...")
init()

generate(sitesDataPath)
print("%d/%d urls" % (count, len(map_url_id)))
print("%d documents" % docFileCount)
