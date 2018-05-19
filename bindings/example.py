import ibsen

r = ibsen.image_create('output', '../build/libvm.so', '\xff', 1)

assert r == 1

