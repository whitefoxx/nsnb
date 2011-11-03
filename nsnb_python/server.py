#!/usr/bin/env python
# -*- coding: utf-8 -*-
import socket
import nsnb
try:
    from psyco import full
    full()
except:
    print '''系统上没有安装 psyco 模块，可能导致运行缓慢 '''

def main():

    classifier = nsnb.Nsnb('nsnb.conf')
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('', 10500))
    sock.listen(5)

    try:
        while True:
            newSocket, address = sock.accept()
            while True:
                receivedData = newSocket.recv(8192)
                parameter = receivedData.split()[0]

                if parameter == 'exit':
                    newSocket.close()
                    sock.close()
                    classifier.exit()
                    exit()
                if parameter == '-t':
                    classifier.train(receivedData.split()[2], receivedData.split()[1])
                    newSocket.sendall('done')
                if parameter == '-c':
                    print receivedData.split()[1]
                    (predict, score) = classifier.predict(receivedData.split()[1])
                    # Request to the true label
                    if score > classifier.threshold - classifier.active_tradeoff and score < classifier.threshold + classifier.active_tradeoff:
                        labelReq = 'labelN'
                    else:
                        labelReq = 'noRequest'
                    newSocket.sendall('class=' + predict + ' score=' + str(score) + ' tfile=*' + ' labelReq=' + labelReq)
                if receivedData == 'over':
                    newSocket.close()
                    break
    finally:
        sock.close()

if __name__ == '__main__':
    main()

