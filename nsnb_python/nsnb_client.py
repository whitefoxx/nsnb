#!/usr/bin/env python
# -*- coding: utf-8 -*-
import socket
import sys

PORT = 10500
BUFF_SIZE = 8192

def usage():
    print '''\nThe nsnb client only support three commands now:
(1). python nsnb_client exit                    (send a terminate signal to the server)
(2). python nsnb_client -c mailPath             (classify an email and print the result)
(3). python nsnb_client -t ham(spam) mailPath   (train a mail based on the tag)\n'''

def main():
    if len(sys.argv) < 2:
        usage()
        exit()
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect(('localhost', PORT))
    except:
        print 'Server is not running!'
        exit()

    if sys.argv[1] == 'exit':
        sock.sendall(sys.argv[1])
        exit()

    if sys.argv[1] == '-c':
        sock.send(sys.argv[1] + ' ' + sys.argv[2])
        result = sock.recv(BUFF_SIZE)
        sock.sendall('over')
        print result

    if sys.argv[1] == '-t':
        sock.sendall(sys.argv[1] + ' ' + sys.argv[2] + ' ' + sys.argv[3])
        result = sock.recv(BUFF_SIZE)
        if result == 'done':
            sock.sendall('over')
            exit()
if __name__ == '__main__':
    main()
