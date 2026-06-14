# Axolotl Protocol

A UDP-based messaging protocol combining Reed-Solomon erasure coding and ElGamal encryption to guarantee data integrity and confidentiality without retransmission (no ARQ),
made from scratch.

Once a message is sent, it is never resent. Loss correction is handled entirely by Reed-Solomon.

## How It Works

Raw data is split into 32-byte chunks. Each chunk is ElGamal-encrypted into 1024 bytes (c1 + c2, 512 bytes each), then signed using the Scnhorr Singature (Referred to 
the Fiat-Shamir Signature), then is encoded using Reed-Solomon.

On the receiver side, if a packet doesn't arrive within the timeout window it is treated as lost and its position is zero-filled. Reed-Solomon corrects up to 8 lost packets (25%) per Limb. The recovered ciphertext is then ElGamal-decrypted to recover the original data.


raw data
  ->  split into 32-byte chunks
  -> ElGamal encrypt -> c1 (512 bytes) + c2 (512 bytes)
  -> Sign with FSS.
  -> RS encode -> 3200-byte Limb
  -> split into 32 × 100-byte packets
  -> send over UDP


## Parameters

| Parameter        | Value                        |
|------------------|------------------------------|
| Packet size      | 100 bytes                    |
| Packets per Limb | 32                           |
| Limb size        | 3200 (1600 symbols) bytes    |
| Raw data / Limb  | 32 bytes                     |
| RS field         | GF(2^16)                     |
| RS codeword (n)  | 1600 symbols                 |
| RS data (k)      | 600 symbols                  |
| RS parity (n-k)  | 800 symbols                  |
| Max loss / Limb  | 8 packets (25%)              |
| ElGamal prime    | 4096-bit (see Security)      |
| FSS prime        | 2048-bit                     |

## Project Structure


src/
  axolotl/      ← protocol API (axolotl.h / axolotl.c)
  elgamal/      ← ElGamal encryption
  rs/           ← Reed-Solomon over GF(2^16)
  fss/          ← Fiat-Shamir Signature
tests/
  sender.c      ← test sender harness
  receiver.c    ← test receiver harness
  raw_test.h    ← test header for sending without protocol
  receiver_raw.c ← receiver without protocol
  sender_raw ← sender without protocol


## Dependencies

- gcc
- libgmp — for Big integer arithmetic

**Rocky Linux / RHEL:**

bash
sudo dnf install gmp-devel


**Debian / Ubuntu:**

bash
sudo apt install libgmp-dev


## Build


bash
make


Produces sender and receiver binaries at the project root.

## Usage

Keys are generated at runtime. The receiver generates a keypair and prints its public key. The sender takes that public key as a command-line argument.

Parameters and keys for FSS are also passed similarly to Elgamal.

**On the receiver machine first:**

bash
./receiver
# prints public key, then waits for parameters to be pasted.


**On the sender machine — paste the printed public key:**

bash
./sender <pkey_hex>
# prints out parameters for FSS to be pasted to receiver and connection begins.


## Security Notes and Further Details
This project has no connection in any way to the Double-Rachet Algorithm (previously known as the Axolotl Algorithm).

While many of the v0.1 security holes were fixed, some remain (See PROTOCOL.md).
- No authenticated key exchange; public keys are exchanged out-of-band.
- No perfect forward secrecy.
- GNU MP is not designed to resist all side-channel attacks.
- Messages are independently authenticated per Limb.
- Experimental implementation; not audited.

Consider this a warning and a reminder that while the project tries to follow modern security standards, it is 
still in an experimental stage and is a project that begun first as a learning project. 
I highly advise against using this for any real world applications.


## Acknowledgements

Tested on the [ProLUG]([https://discord.gg/23dQAQ42e]) lab environment
a community-run Linux upskilling lab. Thanks to the ProLUG community 
for the test infrastructure and feedback.

i have this read me. see any mistakes and see how it can be improved in line to the protocol.md

 