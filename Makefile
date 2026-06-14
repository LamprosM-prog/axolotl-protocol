CC     = gcc
CFLAGS = -Wall -Wextra -g -I./src/axolotl -I./src/elgamal -I./src/rs -I./src/fss
LIBS   = -lgmp

SRC = src/axolotl/axolotl.c \
      src/elgamal/elgamal.c \
      src/rs/gf.c \
      src/rs/poly.c \
      src/rs/rs_decode.c \
      src/rs/rs_encoder.c \
      src/fss/fss.c \
      src/fss/sha256/sha256.c

all: sender receiver sender_raw receiver_raw

sender: tests/sender.c $(SRC)
	$(CC) $(CFLAGS) -o sender tests/sender.c $(SRC) $(LIBS)

receiver: tests/receiver.c $(SRC)
	$(CC) $(CFLAGS) -o receiver tests/receiver.c $(SRC) $(LIBS)

sender_raw: tests/sender_raw.c
	$(CC) $(CFLAGS) -o sender_raw tests/sender_raw.c

receiver_raw: tests/receiver_raw.c
	$(CC) $(CFLAGS) -o receiver_raw tests/receiver_raw.c

clean:
	rm -f sender receiver sender_raw receiver_raw