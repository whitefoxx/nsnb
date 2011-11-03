#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys, os
###############################################
#
#   This is the first part,for parse text
#
###############################################
if len(sys.argv) == 1:
    print """\n[Usage]:\nOnly one parameter: trec official output file
[example]: \n./trec-eval result

It will show you the full results and generate the ROC image in the same folder
"""
    exit()
if len(sys.argv) > 3:
    print 'The number of args not right, CHECK that!'
    exit()
result = file(sys.argv[1], 'r')
corpus = {}
for line in result:
    if line.startswith('#'):
        continue
    temp = line.split()
    for token in temp:
        if token.startswith('score'):
            score = float(token.split('=')[1])
        if token.startswith('judge'):
            if token.split('=')[1] == 'spam':
                label = 1
            else:
                label = -1
    corpus[score] = label
result.close()

###############################################
#
#       Sort the working set
#
##############################################
scores = corpus.keys()
scores.sort()

# Total mail records
total_p, total_n = 0.0, 0.0
for key in corpus.keys():
    if corpus[key] == 1:
        total_p += 1
    else:
        total_n += 1
p, n = total_p, total_n

###############################################
#
#       Main body to caculate
#
##############################################
control = 1000
result = []
result.append((n/total_n, p/total_p))
s = 0
j = 0
i = 0
if len(sys.argv) == 2:
    trade_off = 30
else:
    trade_off = int(sys.argv[2])
outer_min = trade_off * n + (total_p - p)
outer_p = total_p - p
outer_n = n
reliable_threshold = 0
while i < len(scores):
    if corpus[scores[i]] == 1:
        p -= 1
    else:
        n -= 1
    result.append((n/total_n, p/total_p))
    if n == int(total_n/1000):
        xxx = 1 - p/total_p
    i += 1
    inner_min = trade_off * n + (total_p-p) 
    if inner_min < outer_min:
        outer_min = inner_min
        outer_p = total_p - p
        outer_n = n
        reliable_threshold = scores[i-1]

###############################################
#
#       Store the result
#
##############################################
#roc = file('roc', 'w')
#roc.write('# This file for generate roc curve\n')
#for pair in result:
#    roc.write(str(pair[0]) + ' ' + str(pair[1]) + '\n')
#roc.close()

##############################################
#
#       Plot The ROC Curve(Gnuplot)
#
###############################################
#import Gnuplot
#g = Gnuplot.Gnuplot()
#g.plot("""[0:0.05] [1:0.95] 'roc' with lines""")
#g("""set term png""")
#g("""set output 'ROC.png'""")
#g.replot()

##############################################
#
#       Caculate the 1-ROCA
#
##############################################
_result = []
for pair in result:
    temp = []
    temp.append(pair[0]) 
    temp.append(pair[1])
    _result.append(temp)
result = _result
t1 = result.pop(0)
roca = 0.0
for pair in result:
    t2 = pair
    roca += (t1[1]+t2[1]) * (t1[0]-t2[0]) / 2 
    (t1[0], t1[1]) = (t2[0], t2[1])


#############################################
#
#       The other parameter
#
############################################
result = file(sys.argv[1], 'r')
hh, hs, sh, ss = 0.0, 0.0, 0.0, 0.0
for line in result:
    temp = line.split()
    for token in temp:
        if token.startswith('judge'):
            orig = token.split('=')[1]
        if token.startswith('class'):
            pred = token.split('=')[1]
    if orig == 'spam':
        if pred == 'spam':
            ss += 1
        else:
            sh += 1
    else:
        if pred == 'spam':
            hs += 1
        else:
            hh += 1
          
result.close()
import math
# Smoothed parameter
fp = (hs + 0.5) / (hs+hh + 0.5)
fn = (sh + 0.5) / (sh+ss + 0.5)
precision = (hh+ss) / (hh+hs+sh+ss)

_fp = (outer_n + 0.5) / (hs+hh + 0.5)
_fn = (outer_p + 0.5) / (sh+ss + 0.5)
_precision = 1 - (outer_n+outer_p) / (hh+hs+sh+ss)

def logit(x):
    return math.log(x/(1-x))
def invlogit(x):
    return 1/(1+math.exp(-x))

lam = invlogit((logit(fp) + logit(fn))/2)
_lam = invlogit((logit(_fp) + logit(_fn))/2)

#############################################
#    
#       Results Show
#
#############################################
print """\nNow Let's see the Result\n"""
print "ham  ----> ham  : " + str(int(hh))
print "ham  ----> spam : " + str(int(hs))
print "spam ----> ham  : " + str(int(sh))
print "spam ----> spam : " + str(int(ss))
print "TOTAL EMAIL     : " + str(int(hs+sh+ss+hh)) + '\n'
print "false positive rate(%) : " + str(fp*100) + ' %'
print "false negtive rate(%)  : " + str(fn*100) + ' %'
print "precision(%)           : " + str(precision*100) + ' %' + '\n'
print "optimizable threshold  : " + str(reliable_threshold) + " with " + str(int(outer_n)) + " : " + str(int(outer_p))  
print "prime precision(%)     : " + str(_precision*100) + ' %' 
print "prime LAM(%)           : " + str(_lam*100) + ' %\n'
print "LAM(%)                 : " + str(lam*100) + ' %'
print "H=.1 (%)               : " + str(xxx*100) + ' %'
print "1-ROCA(%)              : " + str((1-roca)*100) + ' %\n'
