import math
import cPickle as pickle
import socket
from psyco import full
full()

db = open('oneLayer.db', 'r')
data_structure = pickle.load(db)
(header_confidence_t, body_confidence_t) = (data_structure[4], data_structure[5])
db.close()

header_threshold = [1e-4, 1e4]
body_threshold = [1e-3, 1e2]
total = 0
(header_ham, header_spam, body_ham, body_spam) = ({}, {}, {}, {})
(header_confidence, body_confidence) = ({}, {})

for token in header_confidence_t.keys():
    if header_confidence_t[token] > header_threshold[1] or header_confidence_t[token] < header_threshold[0]:
        header_ham[token] = 0.0
        header_spam[token] = 0.0
        header_confidence[token] = 1.0
        total += 1
for token in body_confidence_t.keys():
    if body_confidence_t[token] > body_threshold[1] or body_confidence_t[token] < body_threshold[0]:
        body_ham[token] = 0.0
        body_spam[token] = 0.0
        body_confidence[token] = 1.0
        total += 1
        
print 'Loads ' + str(total) + ' features...'
        
class Setting(object):
    max_read_len = 4000                 # single mail's max read length
    total_ham = 0.0                     # number of total ham email
    total_spam = 0.0                    # number of total spam email
    ngram = 5                           # ngram
    threshold = 0.50
    thickness = 0.25
    learning_rate = 0.9
    adjust = 200
    smooth = 1e-1
    header_weight = 0.60
    tradeoff = 0.0
    active_tradeoff = 0.25

def logist(x):
    """ The logist function """
    return 1.0 / (1.0 + math.exp(-x))

def tokenize(s):
    """ Input a string, and output a set, which includes all the
        tokens of the given string """
    hashset = set()
    if s == '':
        return hashset
    for i in xrange(len(s) - Setting.ngram):
        hashset.add(s[i:i + Setting.ngram])
    return hashset

def transform(email_path):
    """ Given an email path, and then output two sets of tokens,
        one for email header token set, the other for email body token set"""
    mail = file(email_path, 'r')
    content = mail.read(Setting.max_read_len)
    i = 0
    while not(content[i] == '\n' and content[i + 1] == '\n') and i < len(content) - 5:
        i += 1
    header = content[:i]
    # what for?
    body = content[i + 2:]
    if len(body) + len(header) > Setting.max_read_len:
        body = body[:max(1000, Setting.max_read_len - len(header))]
    header_set = tokenize(header)
    body_set = tokenize(body)
    mail.close()
    return (header_set, body_set)

def train_a_mail(email_path, tag):
    """
    Train a single email for one time, it's always called several times
    """
    (header_set, body_set) = transform(email_path)
    if tag == 'ham':
        Setting.total_ham += 1
        for token in header_set:
            if header_ham.has_key(token):
                header_ham[token] += 1.0
        for token in body_set:
            if body_ham.has_key(token):
                body_ham[token] += 1.0
    else:
        Setting.total_spam += 1
        for token in header_set:
            if header_spam.has_key(token):
                header_spam[token] += 1.0
        for token in body_set:
            if body_spam.has_key(token):
                body_spam[token] += 1.0

def calculate(header_set, body_set):
    """The most important part,
       the calculation stratagy. """
    header_score = 0.0
    body_score = 0.0
    for token in header_set:
        if not(header_ham.has_key(token)):
            continue
        header_score += math.log((header_spam[token] + Setting.smooth) / \
            (header_ham[token] + Setting.smooth) * \
            (Setting.total_ham + 2 * Setting.smooth) / \
            (Setting.total_spam + 2 * Setting.smooth) * \
                header_confidence[token])
    for token in body_set:
        if not(body_ham.has_key(token)):
            continue
        body_score += math.log((body_spam[token] + Setting.smooth) / \
            (body_ham[token] + Setting.smooth) * \
            (Setting.total_ham + 2 * Setting.smooth) / \
            (Setting.total_spam + 2 * Setting.smooth) * \
                body_confidence[token])
    score = header_score * Setting.header_weight + body_score * (1.0 - Setting.header_weight)
    score += math.log((Setting.total_spam + Setting.smooth) / \
        (Setting.total_ham + Setting.smooth))
    return logist(score / Setting.adjust)
    
def predict_without_train(email_path):
    """Simply predict"""
    (header_set, body_set) = transform(email_path)
    score = calculate(header_set, body_set)
    if score > Setting.threshold + Setting.tradeoff:
        predict = 'spam'
    else:
        predict = 'ham'
    return (predict, score)

def predict_a_mail(email_path, tag):
    """Train"""
    (header_set, body_set) = transform(email_path)
    score = calculate(header_set, body_set)
    
    if tag == 'ham':
        Setting.total_ham += 1
    else:
        Setting.total_spam += 1
    
    while tag == 'ham' and score > Setting.threshold - Setting.thickness:
        train_a_mail(email_path, tag)
        Setting.total_ham -= 1
        for token in header_set:
            if not header_confidence.has_key(token):
                continue
            header_confidence[token] *= Setting.learning_rate
    #TODO Maybe here learning rate could be variable! and that's a more natural way.
        for token in body_set:
            if not body_confidence.has_key(token):
                continue
            body_confidence[token] *= Setting.learning_rate
        score = calculate(header_set, body_set)
    while tag == 'spam' and score < Setting.threshold + Setting.thickness:
        train_a_mail(email_path, tag)
        Setting.total_spam -= 1
        for token in header_set:
            if not header_confidence.has_key(token):
                continue
            header_confidence[token] /= Setting.learning_rate
        for token in body_set:
            if not body_confidence.has_key(token):
                continue
            body_confidence[token] /= Setting.learning_rate
        score = calculate(header_set, body_set)
            
def main():

    print 'The nsnb server initialized....'
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('', 10500))
    sock.listen(5)

    try:
        while True:
            newSocket, address = sock.accept()
            #print 'Connect from', address
            while True:
                receivedData = newSocket.recv(8192)
                parameter = receivedData.split()[0]

                if parameter == 'exit':
                    newSocket.close()
                    #print 'Disconnect from', address
                    sock.close()
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
                    #print 'Disconnect from', address
                    newSocket.close()
                    break
    finally:
        sock.close()

if __name__ == '__main__':
    main()

