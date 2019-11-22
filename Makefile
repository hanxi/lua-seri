.PHONY : all win clean

all : linux
win : seri.dll

# For Linux
linux:
	make seri.so "DLLFLAGS = -shared -fPIC -std=c99"
# For Mac OS
macosx:
	make seri.so "DLLFLAGS = -bundle -undefined dynamic_lookup"

seri.so : seri.c
	env gcc -O2 -Wall $(DLLFLAGS) -o $@ $^

seri.dll : seri.c
	gcc -O2 -Wall --shared -o $@ $^ -I/usr/local/include -L/usr/local/bin -llua53

clean :
	rm -f seri.so seri.dll
