# Author: John Nemeth
# Description: Qfinal is meant to represent the final version of the project. 
#				Opted to not create object files for simplicity
# Sources: My endless well of knowledge (jokes)
##################################################

Qmain: Qserver.c Qtopics.c pub sub
	gcc -g -Wall -pthread Qtopics.c Qserver.c -o Qmain -D_GNU_SOURCE

# when running, first argument is publisher count, second is subscriber count
runQ: Qmain
	@echo OUTPUTS TO scen5.out
	./Qmain 5 1 > scen5.out

pub: pub.c
	gcc -g -Wall pub.c -o pub

sub: sub.c
	gcc -g -Wall sub.c -o sub

clean:
	rm -rf sub pub Qmain
