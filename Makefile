CFLAGS=-Wall -Werror -lssl -lcrypto

default: testcase1 testcase2 testcase3 testcase4 base


base: base.c filesys.c
	gcc -o base base.c filesys.c $(CFLAGS)

testcase1: filesys.c testcase1.c base
	gcc -o testcase1 testcase1.c filesys.c $(CFLAGS)

testcase2: filesys.c testcase2.c base
	gcc -o testcase2 testcase2.c filesys.c $(CFLAGS)

testcase3: filesys.c testcase3.c
	gcc -o testcase3 testcase3.c filesys.c $(CFLAGS)

testcase4: filesys.c testcase4.c
	gcc -o testcase4 testcase4.c filesys.c $(CFLAGS)

run:
	./base
	./testcase1
	./base
	./testcase2
	./base
	./testcase3
	./base
	./testcase4

clean:
	rm -rf testcase1 testcase2 testcase3 testcase4 base *.txt
