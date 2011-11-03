import sys

corpus_path = sys.argv[1]
index_path = corpus_path + 'index'
index_file = open(index_path, 'r')

feature_dict = {}

def tokenize(s):
    """ Input a string, and output a set, which includes all the
        tokens of the given string """
    hashset = set()
    if s == '':
        return hashset
    for i in xrange(len(s) - 5):
        hashset.add(s[i:i + 5])
    return hashset

def transform(email_path):
    """ Given an email path, and then output two sets of tokens,
        one for email header token set, the other for email body token set"""
    mail = file(email_path, 'r')
    content = mail.read(4000)
    i = 0
    while not(content[i] == '\n' and content[i + 1] == '\n') and i < len(content) - 5:
        i += 1
    header = content[:i]
    # what for?
    body = content[i + 2:]
    if len(body) + len(header) > 4000:
        body = body[:max(1000, 4000 - len(header))]
    header_set = tokenize(header+body)
    mail.close()
    return header_set
    
libsvm_file = open('libsvmformat', 'w')    
total = 1

for line in index_file:
    (tag, email_path) = line.split()
    email_path = corpus_path + email_path
    feature_set = transform(email_path)
    temp_string = ''
    if tag == 'spam':
        temp_string += '1'
    else:
        temp_string += '-1'
    for feature in feature_set:
        if feature_dict.has_key(feature):
            pass
        else:
            feature_dict[feature] = total
            total += 1
        temp_string += ' ' + str(feature_dict[feature]) + ':1' 
    temp_string += '\n'
    libsvm_file.write(temp_string)
    print 'One mail parsed...'
    
index_file.close()
libsvm_file.close()
            