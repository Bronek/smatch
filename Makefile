define uniq =
  $(eval seen :=)
  $(foreach _,$1,$(if $(filter $_,${seen}),,$(eval seen += $_)))
  ${seen}
endef

CC=clang++
CFLAGS=-Wall -Wextra -std=c++14 -O3 -DNDEBUG -ggdb $(addprefix -D, $(define))
SOURCES=input.cpp book.cpp
TEST=test.cxx
TOBJ=$(SOURCES:.cpp=.o) $(TEST:.cxx=.o)
MAIN=main.cpp
MOBJ=$(SOURCES:.cpp=.o) $(MAIN:.cpp=.o)
OBJECTS=$(call uniq, $(MOBJ) $(TOBJ))
TARGET=smatch

all: $(TARGET) test.log

clean:
	rm -f ./*.o; rm -f ./*.gch; rm -f ./test.log; rm -f ./test; rm -f ./smatch; rm -f ./.depend;

depend: .depend

.depend: $(SOURCES) $(MAIN) $(TEST)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ -MF ./.depend;

include .depend

$(OBJECTS): $(SOURCES) $(MAIN) $(TEST)
	$(CC) -c $(CFLAGS) $(SOURCES) $(MAIN) $(TEST);

$(TARGET): $(MOBJ)
	$(CC) $(CFLAGS) $^ -o $@;

test.log: ./test
	./test | tee test.log;

test: $(TOBJ)
	$(CC) $(CFLAGS) $^ -o $@ -lboost_unit_test_framework -lboost_test_exec_monitor;
