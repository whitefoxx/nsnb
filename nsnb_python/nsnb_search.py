#!/usr/bin/env python

r"""
Search for the most attractive threshold which maximize the 
         lam(%) 
"""

import sys

def usage():
    """demonstrate how to use this script to search a most suitabe threshold"""
    print u'''[usage]: ./search resultFile 
    
[format]: the result file should be follow the TREC result format:
        filepath class=* judge=* score=*
        ...'''
        
def version():
    """print out the version information"""
    print '''nsnb_search version 0.97 (freiz)
freizsu@gmail.com'''

search_area = (0.3, 0.7)

if len(sys.argv) < 2:
    usage()
    exit(1)

file_path = sys.argv[1]

if file_path == "-help" or file_path == "--help":
    usage()
    exit()
    
if file_path == "-v" or file_path == "--version":
    version()
    exit()

try:
    inFile = open(file_path, 'r')
except:
    print "Could not open the result file, please check the path"
    exit()

l = []
t = {}

for line in inFile:
    words = line.split()
    for word in words:
        temp = word.split('=')
        if temp[0] == 'judge':
            judge = temp[1]
        elif temp[0] == 'score':
            score = float(temp[1])
    t[score] = judge

l = t.keys()
l.sort()

total_ham = 0
total_spam = 0

for key in l:
    if t[key] == 'spam':
        total_spam += 1
    else:
        total_ham += 1

minLam = 100

from math import log, exp
def logit(x):
    return log(x / (1.0 - x))
def invlogit(x):
    return 1.0 / (1.0 + exp(-x))
def getLam(ham_error, spam_error):
    fp = (ham_error + 0.5) / (total_ham + 0.5)
    fn = (spam_error + 0.5) / (total_spam + 0.5)
    return invlogit((logit(fp) + logit(fn)) / 2.0) * 100
def fp(x):
    return (x + 0.5) / (total_ham + 0.5) * 100
def fn(x):
    return (x + 0.5) / (total_spam + 0.5) * 100

ham_error = total_ham
spam_error = 0

result = 0
opt_spam_error = 0
opt_ham_error = 0
opt_fp = 0
opt_fn = 0

outFile = open('log', 'w')

for i in xrange(len(l)):
    if t[l[i]] == 'spam':
        spam_error += 1
    else:
        ham_error -= 1

    if l[i] < search_area[0] or l[i] > search_area[1]:
        continue

    lam = getLam(ham_error, spam_error)
    outFile.write(str(l[i]) + ' ' + str(lam) + ' ' + str(int(ham_error)) + ' ' + ' ' + str(int(spam_error)) + ' ' + '\n')
    if lam < minLam:
        opt_spam_error = spam_error
        opt_ham_error = ham_error
        opt_fp = (opt_ham_error + 0.5) / (total_ham + 0.5) * 100
        opt_fn = (opt_spam_error + 0.5) / (total_spam + 0.5) * 100
        result = l[i]
        minLam = lam

print 'Optimal threshold : ' + str(result) 
print '''(lam)%            : ''' + str(minLam)
print 'Ham error         : ' + str(opt_ham_error)
print 'Spam error        : ' + str(opt_spam_error)



