CC     = gcc
CFLAGS = -Wall -Wextra -g -I./src/axolotl -I./src/elgamal -I./src/rs
LIBS   = -lgmp

SRC = src/axolotl/axolotl.c \
      src/elgamal/elgamal.c \
      src/rs/gf.c \
      src/rs/poly.c \
      src/rs/rs_decode.c \
      src/rs/rs_encoder.c

all: sender receiver

sender: tests/sender.c $(SRC)
	$(CC) $(CFLAGS) -o sender tests/sender.c $(SRC) $(LIBS)

receiver: tests/receiver.c $(SRC)
	$(CC) $(CFLAGS) -o receiver tests/receiver.c $(SRC) $(LIBS)

clean:
	rm -f sender receiver