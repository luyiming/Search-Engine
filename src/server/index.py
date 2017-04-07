#-*- coding: UTF-8 -*-
from flask import Flask
from flask import request
from flask import render_template

import requests
from bs4 import BeautifulSoup

import copy
import query
import os
import time
import re
import string

app = Flask(__name__)

app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 0

class SearchResult(object):
    pass

server = query.Server('obj/search')
print('C++ Server starting...')

buf = b'\n'
while b"$" not in buf:
    r = server.recv()
    if r:
        buf += r
print(buf.strip())

def get_result(s):
    server.send(s)
    buf = b'\n'
    while b"$" not in buf:
        r = server.recv()
        if r:
            buf += r
    return buf.decode('utf-8').strip()
    #return unicode(buf, errors='ignore')

@app.route('/')
def index_page():
    return render_template('index.html', name = request.method)

@app.route('/search')
def search():
    search_time = time.clock()
    results = []

    regex = re.compile('[%s\s]' % re.escape(string.punctuation))

    keyword = request.args.get('key', '')

    res = get_result(keyword)
    res = res.split('\n')

    results_size = int(res[0])

    count = 0
    item_count = 0
    item = SearchResult()
    for line in res[1:]:
        if line.strip() == "#":
            item.id = item_count
            item_count = item_count + 1
            results.append(item)
            item = SearchResult()
            count = 0
        if count == 1:
            item.href = line
        elif count == 2:
            item.head = line
        elif count == 3:
            item.rank = float(line)
        elif count == 4:
            item.content = line.strip()
            render_words = regex.sub('', keyword)
            for word in render_words:
                item.content = item.content.replace(word, u"<span class=\"w3-text-red\">" + word + "</span>")
                item.head = item.head.replace(word, u"<span class=\"w3-text-red\">" + word + "</span>")
        count = count + 1
    results = results[1:]
    pages = []
    for i in range(int((item_count - 1)/10) + 1):
        pages.append(i + 1)
    search_time = time.clock() - search_time
    search_time = "%s" % search_time
    return render_template('search.html', key = keyword, results = results, results_size = results_size, time = search_time, pages = pages)
