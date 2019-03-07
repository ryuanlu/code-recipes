TEST_SOURCES:=test.c vector.c
TEST_BINARY:=test
TEST_CFLAGS:=$(CFLAGS) -Wall
TEST_LDFLAGS:=$(LDFLAGS)

all: $(TEST_BINARY)

clean:
	rm -f $(TEST_BINARY) *.o

TEST_OBJECTS:=$(TEST_SOURCES:.c=.o)

$(TEST_OBJECTS): %.o: %.c
	$(CC) -c $< -o $@ $(TEST_CFLAGS)

$(TEST_BINARY): $(TEST_OBJECTS)
	$(CC) $^ -o $@ $(TEST_LDFLAGS)
