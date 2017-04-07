#-*- coding: UTF-8 -*-
import requests
from bs4 import BeautifulSoup
import threading
import queue
import os
import sys
import datetime

end_crawling = False

visited_urls = set()
error_urls = set()
task_queue = queue.Queue()

visited_urls_lock = threading.Lock()
error_urls_lock = threading.Lock()
output_lock = threading.Lock()
log_lock = threading.Lock()

filter_url = ['javascript:;', '#', 'javascript:void(0)']

sites_path = '../../sites/'

exit_flag = False
exit_num = 0
exit_time = None

def init():
    global exit_num
    global end_crawling
    end_crawling = False

    print('Loading data...')
    if not os.path.exists(sites_path):
        os.mkdir(sites_path)
    if os.path.exists('log.txt'):
        with open('log.txt', 'w') as f:
            f.write('')
    if os.path.exists('task_queue.txt'):
        with open('task_queue.txt', 'r') as f:
            for line in f.readlines():
                if line.strip() != '':
                    task_queue.put(line.strip())
    if os.path.exists('visited_urls.txt'):
        with open('visited_urls.txt', 'r') as f:
            f.readline()
            for line in f.readlines():
                if line.strip() != '':
                    visited_urls.add(line.strip())
    if os.path.exists('error_urls.txt'):
        with open('error_urls.txt', 'r') as f:
            f.readline()
            for line in f.readlines():
                if line.strip() != '':
                    error_urls.add(line.strip())
                    #visited_urls.add(line.strip())
    print('task pages: %d' % task_queue.qsize())
    print('visited pages: %d' % len(visited_urls))
    print('error urls: %d' % len(error_urls))
    exit_num = input('additional pages: ')
    exit_num = int(exit_num) + len(visited_urls)

def clean_up():
    with open('task_queue.txt', 'w') as f:
        while not task_queue.empty():
            url = task_queue.get()
            f.write(url + '\n')
    with open('visited_urls.txt', 'w') as f:
        f.write(str(len(visited_urls)) + '\n')
        for url in visited_urls:
            f.write(url + '\n')
    with open('error_urls.txt', 'w') as f:
        f.write(str(len(error_urls)) + '\n')
        for url in error_urls:
            f.write(url + '\n')
    with open('base_urls.txt', 'w') as f:
        for url in base_urls:
            f.write(url + '\n')

def get_text(soup):
    text = ''
    for child in soup.descendants:
        if child.string and child.name and child.name != 'style' and child.name != 'script':
            text = text + child.string.strip() + ' '
    return text.strip()

def exit_signal():
    global exit_flag
    global exit_num
    if exit_flag or len(visited_urls) >= exit_num:
        return True
    else:
        return False

def log(str):
    with log_lock:
        with open('log.txt', 'a') as f:
            f.write(str + '\n')

def log_screen(str):
    with output_lock:
        print(str)

def request_page(url):
    try_times = 1
    while True:
        if try_times > 3:
            with error_urls_lock:
                error_urls.add(url)
            return (False, )
        try:
            req = requests.get(url, timeout = 5, stream = True)
        except Exception as e:
            log('request_page ERROR %s - %s tring %d times' % (e, url, try_times))
            try_times = try_times + 1
            print('ERROR %s tring %d times' % (url, try_times))
            continue
        '''
        if not req.ok or req.url in visited_urls: # for redirects
            continue
        '''
        return (True, req)

class Page(object):
    def __init__(self):
        self.status = False
        self.type = ''
        self.url = ''
        self.title = ''
        self.text = ''
        self.link = []

def guess_encoding(soup, charset):
    for meta in soup.find_all('meta', content = True):
        if 'charset=gb2312' in meta.get('content'):
            return 'gb2312'
    return 'utf-8'

def parse_page(url, req):
    global end_crawling
    headers = req.headers
    base_url = '/'.join(url.split('/')[:3])
    #base_url = url[ : url.find('.cn') + len('.cn')] # without ending slash
    page = Page()
    page.url = url
    content_type = headers.get('content-type')
    if content_type is None:
        log('empty content-type: ' + url)
        return page
    if 'text/html' in content_type:
        content = req.content
        soup = BeautifulSoup(content, 'html.parser')

        #req.encoding = guess_encoding(soup, req.encoding)
        for meta in soup.find_all('meta', {'name':True, 'content':True}):
            if 'keyword' in meta.get('name'):
                page.text = page.text + meta.get('content')
            if 'description' in meta.get('name'):
                page.text = page.text + meta.get('content')

        page.type = 'html'
        page.text = get_text(soup)
        try:
            page.title = soup.title.string.strip()
        except:
            page.title = url
    elif 'text/xml' in content_type:
        content = req.content
        soup = BeautifulSoup(content, 'xml')
        page.type = 'xml'
        page.text = soup.get_text(strip = True)
        try:
            page.title = soup.title.string.strip()
        except:
            page.title = url
    else:
        return page
    with visited_urls_lock:
        visited_urls.add(url)
    if task_queue.qsize() > 300000:
        end_crawling = True
    if not end_crawling:
        for link in soup.find_all('a'):
            url = link.get('href')
            if url and url.strip() not in filter_url:
                url = url.strip()
                if 'http://' not in url:
                    if url[0] == '/':
                        url = base_url + url
                    else:
                        url = base_url + '/' + url
                url = convert_utl(url)
                page.link.append(url)
    else:
        page.link = []
    page.status = True
    return page

base_urls = set()

def save_page(page):
    file_path = sites_path + page.url.split('/')[2] + '/'

    base_urls.add('/'.join(page.url.split('/')[:3]))

    file_name = page.url.replace('/', '_')
    if not os.path.exists(file_path):
        os.mkdir(file_path)
    with open(file_path + file_name, 'w') as f:
        f.write(page.text)
    with open(file_path + file_name + '.graph', 'w') as f:
        f.write(page.title + '\n')
        f.write(page.url + '\n')
        for url in page.link:
            f.write(url + '\n')

# .doc  application/msword
# .docx application/vnd.openxmlformats-officedocument.wordprocessingml.document
# .pdf  application/pdf
# .xlsx application/vnd.openxmlformats-officedocument.spreadsheetml.sheet
# .xls  application/vnd.ms-excel
# .ppt  application/vnd.ms-powerpoint
# .pptx
# http://sociology.nju.edu.cn/	application/x-shockwave-flash

def valid_url(url):
    white_lists = ['zwzy.cenpar.cn']
    # 'rfd.nju.edu.cn', 'pd.nju.cn', 'mat.nju.edu.cn', 'durp.nju.edu.cn'
    black_lists = ['www.zjnju.org.cn']
    f_w = lambda url: True in [True for x in white_lists if x in url]
    f_b = lambda url: True in [True for x in black_lists if x in url]

    if url is None:
        return False
    if url in visited_urls or f_b(url):
        return False

    base_url = '/'.join(url.split('/')[:3])
    if 'nju' in base_url or f_w(url):
        return True

    try:
        r = requests.head(url, timeout = 3, allow_redirects = True)
    except Exception as e:
        log(e)
        return False

    url = r.url
    base_url = '/'.join(url.split('/')[:3])
    if url in visited_urls or f_b(url):
        return False
    if 'nju' in base_url or f_w(url):
        return True

    return False

# remove hash tag(#) in url
# remove slash tag(/) in root url
# TODO: *.nju.edu.cn:8080/#
def convert_utl(url):
    if url is None:
        return url
    if '#' in url:
        url = url[ : url.find('#')]
    if url[-4:] == '.cn/' or url[-5:] == '.com/' or url[-5:] == '.org/':
        url = url[ : -1]
    return url

def worker():
    url = ''
    while True:
        try:
            if exit_signal():
                log_screen('thread %s >>> exit' % threading.current_thread().name)
                return
            url = task_queue.get()
            if not valid_url(url):
                continue
            log_screen('thread %s >>> downloading %s' % (threading.current_thread().name, url))
            log_screen('thread %s >>> %d pages remaining...'%(threading.current_thread().name, task_queue.qsize()))

            req = request_page(url)
            if req[0] == True:
                req = req[1]
            else:
                continue

            page = parse_page(url, req)
            req.close()
            if page.status == False:
                continue
            log_screen('thread %s >>> %d pages visited...'%(threading.current_thread().name, len(visited_urls)))
            for url in page.link:
                if url not in visited_urls:
                    task_queue.put(url)
            save_page(page)
        except Exception as e:
            log("Fatal Error: %s - %s" % (e, url))
            #raise


init()

if exit_signal() == False:
    try:
        threads = []
        for i in range(10):
            threads.append(threading.Thread(target = worker, name = str(i)))
        for t in threads:
            t.start()
        for t in threads:
            t.join()
    except (Exception, KeyboardInterrupt):
        print('waiting threads exit...')
        exit_flag = True
        for t in threads:
            t.join()
    print('Saving data...')
    clean_up()
