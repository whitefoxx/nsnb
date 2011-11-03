#!/usr/bin/env python
# -*- coding: utf-8 -*-

r'NSNB 垃圾邮件过滤算法核心类，实现了train predict 等接口，并且将当前模型存储'

import math
import ConfigParser
import os
import cPickle as pickle
import time
import random
import sys
try:
    from psyco import full
    full()
except:
    print '''系统上没有安装 psyco 模块，可能导致运行缓慢 '''

class Nsnb(object):
    r"""分类准确，运行超快的垃圾邮件过滤算法"""
    def __init__(self, config_file_path):
        
        begin_to_initialize = time.time()
        
        cf = ConfigParser.ConfigParser()
        cf.read(config_file_path)
        self.max_read_len = cf.getint("nsnb", "max_read_len")               
        self.ngram = cf.getint("nsnb", "ngram")
        self.threshold = float(cf.get("nsnb", "threshold"))
        self.thickness = float(cf.get("nsnb", "thickness"))
        self.learning_rate = float(cf.get("nsnb", "learning_rate"))
        self.adjust = float(cf.get("nsnb", "adjust"))
        self.smooth = float(cf.get("nsnb", "smooth"))
        self.header_weight = float(cf.get("nsnb", "header_weight"))
        self.tradeoff = float(cf.get("nsnb", "tradeoff"))
        self.active_tradeoff = float(cf.get("nsnb", "active_tradeoff"))
        self.db_path = cf.get("db", "path")
        
        self.history = {}
        self.history_keys = []
        self.history_size = cf.getint("nsnb", "history_size")
        self.max_history_size = cf.getint("nsnb", "max_history_size")
        self.number_of_trained_mail = 0
        self.prior_confidence = 1.0
        
        if not os.path.isfile(self.db_path):
            (self.header_ham, self.header_spam) = ({}, {})
            (self.body_ham, self.body_spam) = ({}, {})
            (self.header_confidence, self.body_confidence) = ({}, {})
            self.db = []
            self.total_ham = 0.0
            self.total_spam = 0.0
            for i in range(8):
                self.db.append(0)
        else:
            f = open(self.db_path, 'r')
            self.db = pickle.load(f)
            (self.header_ham, self.header_spam) = (self.db[0], self.db[1])
            (self.body_ham, self.body_spam) = (self.db[2], self.db[3])
            (self.header_confidence, self.body_confidence) = (self.db[4], self.db[5])
            (self.total_ham, self.total_spam) = (self.db[6], self.db[7])
            f.close()
        
        print '''Nsnb 初始化完成，耗时：''' + str(time.time()-begin_to_initialize) + r'（秒）'
        
    def logist(self, x):
        """ The logist function """
        return 1.0 / (1.0 + math.exp(-x))
            
    def tokenize(self, s):
        """ Input a string, and output a set, which includes all the
            tokens of the given string """
        hashset = set()
        if s == '':
            return hashset
        for i in xrange(len(s) - self.ngram):
            hashset.add(s[i:i + self.ngram])
        return hashset
        
    def transform(self, email_path):
        """ Given an email path, and then output two sets of tokens,
            one for email header token set, the other for email body token set"""
        mail = open(email_path, 'r')
        content = mail.read(self.max_read_len)
        i = 0
        while not(content[i] == '\n' and content[i + 1] == '\n') and i < len(content) - self.ngram:
            i += 1
        header = content[:i]
        # TODO find a smarter way deal with the header-body problem
        body = content[i + 2:]
        if len(body) + len(header) > self.max_read_len:
            body = body[:max(1000, self.max_read_len - len(header))]
        header_set = self.tokenize(header)
        body_set = self.tokenize(body)
        mail.close()
        return (header_set, body_set)
        
    def train_cell(self, email_path, tag):
        """
        Train a single email for one time, it's always called several times
        """
        (header_set, body_set) = self.transform(email_path)
        if tag == 'ham':
            self.total_ham += 1
            for token in header_set:
                if self.header_ham.has_key(token):
                    self.header_ham[token] += 1.0
                else:
                    self.header_ham[token] = 1.0
                if not(self.header_spam.has_key(token)):
                    self.header_spam[token] = 0.0
                if not(self.header_confidence.has_key(token)):
                    self.header_confidence[token] = 1.0
            for token in body_set:
                if self.body_ham.has_key(token):
                    self.body_ham[token] += 1.0
                else:
                    self.body_ham[token] = 1.0
                if not(self.body_spam.has_key(token)):
                    self.body_spam[token] = 0.0
                if not(self.body_confidence.has_key(token)):
                    self.body_confidence[token] = 1.0
        else:
            self.total_spam += 1
            for token in header_set:
                if self.header_spam.has_key(token):
                    self.header_spam[token] += 1.0
                else:
                    self.header_spam[token] = 1.0
                if not(self.header_ham.has_key(token)):
                    self.header_ham[token] = 0.0
                if not(self.header_confidence.has_key(token)):
                    self.header_confidence[token] = 1.0
            for token in body_set:
                if self.body_spam.has_key(token):
                    self.body_spam[token] += 1.0
                else:
                    self.body_spam[token] = 1.0
                if not(self.body_ham.has_key(token)):
                    self.body_ham[token] = 0.0
                if not(self.body_confidence.has_key(token)):
                    self.body_confidence[token] = 1.0        

    def calculate(self, header_set, body_set):
        """The most important part,
           the calculation stratagy. """
        header_score = 0.0
        body_score = 0.0
        for token in header_set:
            if not(self.header_ham.has_key(token)):
                continue
            header_score += math.log((self.header_spam[token] + self.smooth) / \
                (self.header_ham[token] + self.smooth) * \
                (self.total_ham + 2 * self.smooth) / \
                (self.total_spam + 2 * self.smooth) * \
                    self.header_confidence[token])
        for token in body_set:
            if not(self.body_ham.has_key(token)):
                continue
            body_score += math.log((self.body_spam[token] + self.smooth) / \
                (self.body_ham[token] + self.smooth) * \
                (self.total_ham + 2 * self.smooth) / \
                (self.total_spam + 2 * self.smooth) * \
                    self.body_confidence[token])
        score = header_score * self.header_weight + body_score * (1.0 - self.header_weight)
        score += math.log((self.total_spam + self.smooth) / \
            (self.total_ham + self.smooth) * self.prior_confidence)
        return self.logist(score / self.adjust)
        
    def predict(self, email_path):
        """Simply predict"""
        (header_set, body_set) = self.transform(email_path)
        score = self.calculate(header_set, body_set)
        if score > self.threshold + self.tradeoff:
            predict = 'spam'
        else:
            predict = 'ham'
        return (predict, score)
        
    def train_past(self, email_path, tag):
        (header_set, body_set) = self.transform(email_path)
        score = self.calculate(header_set, body_set)
        while tag == 'ham' and score > self.threshold - self.thickness:
            self.train_cell(email_path, tag)
            self.total_ham -= 1
            for token in header_set:
                self.header_confidence[token] *= self.learning_rate
        #TODO Maybe here learning rate could be variable! and that's a more natural way.
            for token in body_set:
                self.body_confidence[token] *= self.learning_rate
            score = self.calculate(header_set, body_set)
        while tag == 'spam' and score < self.threshold + self.thickness:
            self.train_cell(email_path, tag)
            self.total_spam -= 1
            for token in header_set:
                self.header_confidence[token] /= self.learning_rate
            for token in body_set:
                self.body_confidence[token] /= self.learning_rate
            score = self.calculate(header_set, body_set)

    def train(self, email_path, tag):
        """Train"""
        self.number_of_trained_mail += 1
        if self.number_of_trained_mail <= self.max_history_size:
            self.history_keys.append(email_path)
        else:
            self.history_keys[self.number_of_trained_mail % self.max_history_size] = email_path
        self.history[email_path] = tag
        
        (header_set, body_set) = self.transform(email_path)
        score = self.calculate(header_set, body_set)

        if tag == 'ham':
            self.total_ham += 1
        else:
            self.total_spam += 1

        while tag == 'ham' and score > self.threshold - self.thickness:
            self.train_cell(email_path, tag)
            self.total_ham -= 1
            for token in header_set:
                self.header_confidence[token] *= self.learning_rate
        #TODO Maybe here learning rate could be variable! and that's a more natural way.
            for token in body_set:
                self.body_confidence[token] *= self.learning_rate
            self.prior_confidence *= self.learning_rate
            score = self.calculate(header_set, body_set)
        while tag == 'spam' and score < self.threshold + self.thickness:
            self.train_cell(email_path, tag)
            self.total_spam -= 1
            for token in header_set:
                self.header_confidence[token] /= self.learning_rate
            for token in body_set:
                self.body_confidence[token] /= self.learning_rate
            self.prior_confidence *= self.learning_rate
            score = self.calculate(header_set, body_set)
            
        length = len(self.history_keys)
        if length < 2 * self.history_size:
            return
        for k in range(self.history_size):
            number = random.randint(0, length-1)
            past_email_path = self.history_keys[number]
            self.train_past(past_email_path, self.history[past_email_path])
            
    def exit(self):
        """exit the nsnb spam filter"""
        print 'about to stop, saving the model...'
        f = open(self.db_path, 'w')
        (self.db[0], self.db[1]) = (self.header_ham, self.header_spam)
        (self.db[2], self.db[3]) = (self.body_ham, self.body_spam)
        (self.db[4], self.db[5]) = (self.header_confidence, self.body_confidence)
        (self.db[6], self.db[7]) = (self.total_ham, self.total_spam)
        pickle.dump(self.db, f, True)
        f.close()
        print 'The nsnb spam classifier has been closed!'

##############################################################
#
#  控制邮件过滤训练的流程
#
##############################################################

def main():
    """Self control"""
    corpus_path = sys.argv[1]
    index_path = corpus_path + 'index'
    result_path = sys.argv[2]
    
    index_file = open(index_path, 'r')
    total_email = 0
    for line in index_file:
        total_email += 1
    print '''Let's begin! total email: ''' + str(total_email)
    index_file.close()
    nsnb = Nsnb('nsnb.conf')
    result_file = open(result_path, 'w')
    index_file = open(index_path, 'r')
    
    count = 0
    begin = time.time()
    for line in index_file:
        (tag, email_path) = line.split()
        email_path = corpus_path + email_path
        (predict, score) = nsnb.predict(email_path)
        out = email_path + ' judge=' + tag.lower() + ' class=' + str(predict) + ' score=' + str(score)
        result_file.write(out+'\n') 
        if tag == 'ham' or tag == 'HAM':
            nsnb.train(email_path, 'ham')
        if tag == 'spam' or tag == 'SPAM':
            nsnb.train(email_path, 'spam')
        
        count += 1
        if (count+1) % 500 == 0:
            passed = time.time() - begin
            print r'进度：' + str((count+0.0)/total_email * 100)[:4] + r'%  用时：' + str(passed)[:5] + r'（秒） 还需：' + str( (total_email-count+0.0)/count*passed )[:5] + r'（秒）'
    
    result_file.flush()
    result_file.close()
    index_file.close()
    end = time.time()
    print r'恭喜全部完成！总用时： ' + str(end-begin)[:5] + r'秒 平均一秒处理：' + str(total_email/(end-begin))[:4] + r'封'        
    print r'查看结果：'
    
    os.system('''python trec-eval.py ''' + sys.argv[2])
    label = time.time()
    os.system('cp ' + sys.argv[2] + ' ' + sys.argv[2] + str(label))
    nsnb.exit()
    os.system('mv ' + str(nsnb.db_path) + ' ' + str(nsnb.db_path) + str(label))
    
    
if __name__ == '__main__':
    main()