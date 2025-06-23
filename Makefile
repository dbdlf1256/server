# Target: Dependencies
#	Recipe

CC = gcc
CFLAGS = -W -Wall
# LDFLAGS = 
# LDLIBS = 
OBJS = tcpserver.o udpserver.o protocol.o broad.o main.o
TARGET = testProgram

%.o: %.c
	$(CC) $(CFLAGS) -c $<

# tcpserver: tcpserver.c
# 	$(CC) $(CFLAGS) -c $<

# udpserver: udpserver.c
# 	$(CC) $(CFLAGS) -c $<

# protocol: protocol.c
# 	$(CC) $(CFLAGS) -c $<

# broad: broad.c
# 	$(CC) $(CFLAGS) -c $<

# main: main.c
# 	$(CC) $(CFLAGS) -c $<

${TARGET}: ${OBJS}
	$(CC) $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)