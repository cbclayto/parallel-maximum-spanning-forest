import random

num_nodes = 50000
edge_fraction = 0.00001
expected = int(edge_fraction*num_nodes*(num_nodes-1)/2)
weight_min = 0
weight_max = 10


f = open('graphs/generated/out.random_%d_%d' % (num_nodes, expected), 'w')

f.write('%% %d about %d edges\n' % (num_nodes, expected))
f.write('%% %d %d %d\n' % (expected, num_nodes, num_nodes))

for i in range(1, num_nodes+1):
    for j in range(i+1, num_nodes+1):
        if random.random() < edge_fraction:
            w = random.randrange(weight_min, weight_max)
            f.write('%d %d %d\n' % (i, j, w))

f.close()
