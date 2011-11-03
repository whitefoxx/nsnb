#!/usr/bin/env python

import math
import os
import socket
import cPickle as pickle

class Setting(object):
    max_read_len = 4000
    total_ham = 0.0
    total_spam = 0.0
    ngram = 5
    threshold = 0.50
    thickness = 0.25
    learning_rate = 0.65
    adjust = 22000
    smooth = 1e-5
    split = 10
    upper_learning_rate = 0.995
    tradeoff = -0.09
    active_tradeoff = 0.25

#class corpus(object):
#    def __init__(self, path):
#        self.path = path
#        self.index_path = path + 'index'
#        self.parse()
#    def parse(self):
#        self.map = {}
#        f = file(self.index_path, 'r')
#        for line in f:
#            (judge, name) = line.split()
#            self.map[self.path+name] = judge
#        self.keys = self.map.keys()
#        # If do not shuffle here, we use the canonical order
#        #random.shuffle(self.keys)

# The most important structure during the prossece

if not os.path.isfile('twoLayer.db'):
    data_structure = []
    for i in range(Setting.split):
        data_structure.append([{}, {}, {}, 1.0])
else:
    f = file('twoLayer.db', 'r')
    data_structure = pickle.load(f)
    f.close()
# [{}, {}, {}, 1.0] represent for [ham_set, spam_set, confidence_set, segement_confidence]

# The total result store in the file named 'result'

def logist(x):
    """for scale purpose"""
    return 1.0/(1.0 + math.exp(-x))
 
def tokenize(s):
    """given a specified string, return a set of features extracted from which"""
    token_set = set()
    if s == '':
        return token_set
    for i in xrange(len(s) - Setting.ngram):
        token_set.add(s[i:i+Setting.ngram])
    return token_set

def transform(email_path):
    mail = file(email_path, 'r')
    content_string = mail.read(Setting.max_read_len)
    length = Setting.max_read_len // Setting.split + 1
    blocks = []
    for i in range(Setting.split):
        blocks.append( tokenize( content_string[i*length:(i+1)*length] ) )
    mail.close()
    return blocks

def train_a_mail(email_path, tag):
    blocks = transform(email_path)
    if tag == 'ham':
        Setting.total_ham += 1
        for i in range( Setting.split ):
            for token in blocks[i]:
                if data_structure[i][0].has_key(token):
                    data_structure[i][0][token] += 1.0
                else:
                    data_structure[i][0][token] = 1.0
                if not( data_structure[i][1].has_key(token) ):
                    data_structure[i][1][token] = 0.0
                if not( data_structure[i][2].has_key(token) ):
                    data_structure[i][2][token] = 1.0
    else:
        Setting.total_spam += 1
        for i in range( Setting.split ):
            for token in blocks[i]:
                if data_structure[i][1].has_key(token):
                    data_structure[i][1][token] += 1.0
                else:
                    data_structure[i][1][token] = 1.0
                if not( data_structure[i][0].has_key(token) ):
                    data_structure[i][0][token] = 0.0
                if not( data_structure[i][2].has_key(token) ):
                    data_structure[i][2][token] = 1.0

def calculate(blocks):
    prime_scores = []
    for i in range(Setting.split):
        temp_score = 0.0
        for token in blocks[i]:
            if not( data_structure[i][0].has_key(token) ):
                continue
            temp_score += math.log( (data_structure[i][1][token] + Setting.smooth) / \
                                    (data_structure[i][0][token] + Setting.smooth) * \
                                    (Setting.total_ham + 2 * Setting.smooth) / \
                                    (Setting.total_spam + 2 * Setting.smooth) * \
                                     data_structure[i][2][token] )
        temp_score += math.log( (Setting.total_spam + Setting.smooth) / \
                                (Setting.total_ham + Setting.smooth) )
        prime_scores.append( logist(temp_score/Setting.adjust*Setting.split) )

    final_score = 0.0
    for i in range(Setting.split):
        final_score += math.log( (1.0/prime_scores[i]-1) ** -1 * data_structure[i][3] )
    final_score = logist( final_score )
    prime_scores.append( final_score )
    # print final_score
    return prime_scores

def predict_without_train(email_path):
    blocks = transform(email_path)
    scores = calculate(blocks)
    if scores[-1] > Setting.threshold + Setting.tradeoff:
    	predict = 'spam'
    else:
    	predict = 'ham'
    return (predict, scores[-1])

def predict_a_mail(email_path, tag):
    blocks = transform(email_path)
    scores = calculate(blocks)

    if tag == 'ham':
        Setting.total_ham += 1
    else:
        Setting.total_spam += 1

    while tag == 'ham' and scores[-1] > Setting.threshold - Setting.thickness:
        train_a_mail(email_path, tag)
        Setting.total_ham -= 1
        for i in range(Setting.split):
            if scores[i] > Setting.threshold - Setting.thickness:
                for token in blocks[i]:
                    data_structure[i][2][token] *= Setting.learning_rate
                data_structure[i][3] *= Setting.upper_learning_rate
        scores = calculate(blocks)
    while tag == 'spam' and scores[-1] < Setting.threshold + Setting.thickness:
        train_a_mail(email_path, tag)
        Setting.total_spam -= 1
        for i in range(Setting.split):
            if scores[i] < Setting.threshold + Setting.thickness:
                for token in blocks[i]:
                    data_structure[i][2][token] /= Setting.learning_rate
                data_structure[i][3] /= Setting.upper_learning_rate
        scores = calculate(blocks)

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('', 10500))
    sock.listen(5)

    try:
        while True:
            newSocket, address = sock.accept()
            # print 'Connect from', address
            while True:
                receivedData = newSocket.recv(8192)
                parameter = receivedData.split()[0]

                if parameter == 'exit':
                    newSocket.close()
                    # print 'Disconnect from', address
                    exit()
                if parameter == '-t':
                    predict_a_mail(receivedData.split()[2], receivedData.split()[1])
                    newSocket.sendall('done')
                if parameter == '-c':
                    (predict, score) = predict_without_train(receivedData.split()[1])
                    # need a label to request to the true label
                    if score > Setting.threshold - Setting.active_tradeoff and score < Setting.threshold + Setting.active_tradeoff:
                        labelReq = 'labelN'
                    else:
                        labelReq = 'noRequest'
                    newSocket.sendall('class=' + predict + ' score=' + str(score) + ' tfile=*' + ' labelReq=' + labelReq)
                if receivedData == 'over':
                    newSocket.close()
                    # print 'Disconnect from', address
                    break

    finally:
        sock.close()
        f = file('twoLayer.db', 'w')
        pickle.dump(data_structure, f, True)
        f.close()

if __name__ == "__main__":
    main()
