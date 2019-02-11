# Author: John Nemeth
# Sources: My endless well of knowledge (lies)

P1: P1.o pubp12 subp12
	gcc -g -Wall P1.o -o P1

P2: P2.o pubp12 subp12
	gcc -g -Wall -pthread P2.o -o P2

P3: P3.o pub sub
	gcc -g -Wall -pthread P3.o -o P3

P4: P4server.o P4topics.o pub sub
	gcc -g -Wall -pthread P4topics.o P4server.o -o P4

P6: P6server.o P6topics.o pub6 sub6
	gcc -g -Wall -pthread P6topics.o P6server.o -o P6

QScen1: QserverScen1.o QtopicsScen1.o pubScen1 subScen1
	gcc -g -Wall -pthread QtopicsScen1.o QserverScen1.o -o QScen1

QScen2: QserverScen2.c QtopicsScen2.c pubScen1 subScen1
	gcc -g -Wall -pthread QtopicsScen2.c QserverScen2.c -o QScen2 -D_GNU_SOURCE

QScen3: QserverScen3.c QtopicsScen3.c pubScen3 subScen3
	gcc -g -Wall -pthread QtopicsScen3.c QserverScen3.c -o QScen3 -D_GNU_SOURCE

QScen4: QserverScen4.c QtopicsScen4.c pubScen4 subScen4
	gcc -g -Wall -pthread QtopicsScen4.c QserverScen4.c -o QScen4 -D_GNU_SOURCE

QScen5: QserverScen5.c QtopicsScen5.c pubScen5 subScen5
	gcc -g -Wall -pthread QtopicsScen5.c QserverScen5.c -o QScen5 -D_GNU_SOURCE

runScen1: QScen1
	@echo OUTPUTS TO scen1.out
	./QScen1 1 1 > scen1.out

runScen2: QScen2
	@echo OUTPUTS TO scen2.out
	./QScen2 4 5 > scen2.out

runScen3: QScen3
	@echo OUTPUTS TO scen3.out
	./QScen3 1 1 > scen3.out

runScen4: QScen4
	@echo OUTPUTS TO scen4.out
	./QScen4 3 6 > scen4.out

runScen5: QScen5
	@echo OUTPUTS TO scen5.out
	./QScen5 5 1 > scen5.out

P1.o: P1.c
	gcc -g -Wall -c P1.c -o P1.o

P2.o: P2.c
	gcc -g -Wall -c -pthread P2.c -o P2.o

P3.o: P3.c
	gcc -g -Wall -c -pthread P3.c -o P3.o

P4server.o: P4server.c
	gcc -g -Wall -c -pthread P4server.c -o P4server.o

P4topics.o: P4topics.c
	gcc -g -Wall -c -pthread  P4topics.c -o P4topics.o -D_GNU_SOURCE

P6server.o: P6server.c
	gcc -g -Wall -c -pthread P6server.c -o P6server.o

P6topics.o: P6topics.c
	gcc -g -Wall -c -pthread  P6topics.c -o P6topics.o -D_GNU_SOURCE

QserverScen1.o: QserverScen1.c
	gcc -g -Wall -c -pthread QserverScen1.c -o QserverScen1.o

QtopicsScen1.o: QtopicsScen1.c
	gcc -g -Wall -c -pthread  QtopicsScen1.c -o QtopicsScen1.o -D_GNU_SOURCE

pubp12: pubpart1-2.c
	gcc -g -Wall pubpart1-2.c -o pubpart1-2

subp12: subpart1-2.c
	gcc -g -Wall subpart1-2.c -o subpart1-2

pub: publisher.c
	gcc -g -Wall publisher.c -o publisher

sub: subscriber.c
	gcc -g -Wall subscriber.c -o subscriber

pub6: pub6.c
	gcc -g -Wall pub6.c -o pub6

sub6: sub6.c
	gcc -g -Wall sub6.c -o sub6

pubScen1: pubScen1.c
	gcc -g -Wall pubScen1.c -o pubScen1

subScen1: subScen1.c
	gcc -g -Wall subScen1.c -o subScen1

pubScen3: pubScen3.c
	gcc -g -Wall pubScen3.c -o pubScen3

subScen3: subScen3.c
	gcc -g -Wall subScen3.c -o subScen3

pubScen4: pubScen4.c
	gcc -g -Wall pubScen4.c -o pubScen4

subScen4: subScen4.c
	gcc -g -Wall subScen4.c -o subScen4

pubScen5: pubScen5.c
	gcc -g -Wall pubScen5.c -o pubScen5

subScen5: subScen5.c
	gcc -g -Wall subScen5.c -o subScen5

clean:
	rm -rf P1.o P2.o P3.o P1 P2 P3 publisher subscriber pubpart1-2 subpart1-2 P4server.o P4topics.o P4
	rm -rf P6 P6topics.o P6server.o sub6 pub6 P4Scen1 pubScen1 subScen1 QScen1
	rm -rf *.o QScen2 QScen3 pubScen3 subScen3 
	rm -rf QScen4 subScen4 pubScen4 QScen5 subScen5 pubScen5 
