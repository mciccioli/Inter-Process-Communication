include ./Makefile.inc


all: app vista slave

app: proceso_app_final.c
	gcc $(GCCFLAGS) proceso_app_final.c -o app $(LIBS) 

vista: proceso_vista_final.c
	gcc $(GCCFLAGS) proceso_vista_final.c -o vista $(LIBS) 

slave: proceso_slave_final.c
	gcc $(GCCFLAGS) proceso_slave_final.c -o slave 

clean: 
	rm app vista slave

test: clean
	../pvs.sh
	valgrind -v ./app ../files/* 2> valgrind_app.tasks | ./vista
	./app ../files/*  | valgrind -v ./vista 2> valgrind_vista.tasks
	cppcheck --quiet --enable=all --force --inconclusive proceso_app_final.c 2> cppcheck_app.tasks
	cppcheck --quiet --enable=all --force --inconclusive proceso_vista_final.c 2> cppcheck_vista.tasks
	cppcheck --quiet --enable=all --force --inconclusive proceso_slave_final.c 2> cppcheck_slave.tasks

.PHONY: all clean app vista slave test
