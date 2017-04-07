from subprocess import Popen, PIPE
import fcntl
import os
import time


class Server(object):

    def __init__(self, args, server_env = None):
        if server_env:
            self.process = Popen(args, stdin = PIPE, stdout = PIPE,
                                 stderr = PIPE, env = server_env)
        else:
            self.process = Popen(args, stdin = PIPE, stdout = PIPE, stderr = PIPE)
        flags = fcntl.fcntl(self.process.stdout, fcntl.F_GETFL)
        fcntl.fcntl(self.process.stdout, fcntl.F_SETFL, flags | os.O_NONBLOCK)

    def terminate(self):
        self.process.terminate()

    # None value indicates that the process hasn’t terminated yet.
    def poll(self):
        return self.process.poll()

    def send(self, data, tail='\n'):
        self.process.stdin.write(data.encode('utf-8') + tail.encode('utf-8'))
        self.process.stdin.flush()

    def recv(self, timeout = 0.1, raise_exception = False, tr = 5, stderr = 0):
        time.sleep(timeout)
        if tr < 1:
            tr = 1
        x = time.time() + timeout
        r = ''
        pr = self.process.stdout
        if stderr:
            pr = self.process.stdout
        while time.time() < x or r:
            try:
                r = pr.read()
            except:
                pass
            if r is None:
                if raise_exception:
                    raise Exception("No message")
                else:
                    break
            elif r:
                return r
            else:
                time.sleep(max((x - time.time()) / tr, 0))
        if r:
            return r
        else:
            return None


if __name__ == "__main__":
    server = Server('obj/search')
    print('C++ Server starting...')

    buf = '\n'
    while buf[-1] != "$":
        r = server.recv()
        if r:
            buf += r
    print(buf.strip())

    while True:
        s = input('query string:')
        if s == 'exit':
            server.terminate()
            break

        server.send(s)

        buf = '\n'
        while buf[-1] != "$":
            r = server.recv()
            if r:
                buf += r
        print(buf.strip())
