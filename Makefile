all: myhttpd mycrawler

myhttpd: myhttpd.o QueueImplementation.o
	gcc myhttpd.o QueueImplementation.o -o myhttpd -l pthread

mycrawler: mycrawler.o QueueImplementation.o
	gcc mycrawler.o QueueImplementation.o -o mycrawler -l pthread

myhttpd.o: myhttpd.c
	gcc -c myhttpd.c -l pthread

mycrawler.o: mycrawler.c
	gcc -c mycrawler.c

QueueImplementation.o: QueueImplementation.c
	gcc -c QueueImplementation.c

clean:
	rm -f myhttpd mycrawler myhttpd.o mycrawler.o QueueImplementation.o
