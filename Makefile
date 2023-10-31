TARGET=pipesort
# DEBUG=-g
# WALL=-Wall
# SUPPRESS=-w

all: $(TARGET)

pipesort:
	cc $(DEBUG) $(WALL) -o pipesort pipesort.c

clean:
	rm -f pipesort
