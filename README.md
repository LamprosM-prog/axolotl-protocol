# Axolotl Protocol

A UDP-based messaging protocol combining Reed-Solomon erasure coding, ElGamal encryption,
and Fiat-Shamir/Schnorr (FSS) signatures to provide data confidentiality, integrity, and
authenticity without retransmission (no ARQ) — built from scratch.

Once a message is sent, it is never resent. Loss correction is handled entirely by
Reed-Solomon; tampering is detected (not corrected) via FSS signatures.

## How It Works

Raw data is split into 32-byte chunks. Each chunk is ElGamal-encrypted into 1024 bytes
(c1 + c2, 512 bytes each), then `c1 || c2` is signed using the Schnorr signature scheme
via the Fiat-Shamir transform. The ciphertext, signature, and padding form a 1600-byte
frame, which is Reed-Solomon encoded into a 3200-byte Limb.

On the receiver side, if a packet doesn't arrive within the timeout window it is treated
as lost and its position is zero-filled. Reed-Solomon corrects up to 8 lost packets (25%)
per Limb. If correction succeeds, the signature is verified before decryption — a Limb
that fails verification is never decrypted. Each Limb is reported back to the caller as
`OK`, `LOST`, or `TAMPERED` (see PROTOCOL.md).

```
raw data
  -> split into 32-byte chunks
  -> ElGamal encrypt -> c1 (512 bytes) + c2 (512 bytes)
  -> sign c1||c2 with FSS -> e (32 bytes) + s (256 bytes)
  -> frame = c1 + c2 + e + s + padding (1600 bytes)
  -> RS encode -> 3200-byte Limb
  -> split into 32 x 100-byte packets
  -> send over UDP
```

## Parameters

| Parameter        | Value                        |
|------------------|------------------------------|
| Packet size      | 100 bytes                    |
| Packets per Limb | 32                           |
| Limb size        | 3200 bytes (1600 symbols)    |
| Raw data / Limb  | 32 bytes                     |
| RS field         | GF(2^16)                     |
| RS codeword (n)  | 1600 symbols                 |
| RS data (k)      | 800 symbols                  |
| RS parity (n-k)  | 800 symbols                  |
| Max loss / Limb  | 8 packets (25%)              |
| ElGamal prime    | 4096-bit (see Security)      |
| FSS prime        | 2048-bit (see Security)      |

## Project Structure
```
src/
  axolotl/       <- protocol API (axolotl.h / axolotl.c)
  elgamal/       <- ElGamal encryption
  rs/            <- Reed-Solomon over GF(2^16)
  fss/           <- Fiat-Shamir/Schnorr signatures
tests/
  sender.c       <- test sender harness
  receiver.c     <- test receiver harness
  raw_test.h     <- shared header for protocol-free packet tests
  sender_raw.c   <- sender without protocol (loss baseline)
  receiver_raw.c <- receiver without protocol (loss baseline)
```

## Dependencies
- gcc
- libgmp — for big integer arithmetic

**Rocky Linux / RHEL:**
```bash
sudo dnf install gmp-devel
```

**Debian / Ubuntu:**
```bash
sudo apt install libgmp-dev
```

## Build
```bash
make
```
Produces `sender`, `receiver`, `sender_raw`, and `receiver_raw` binaries at the project root.

## Usage

Keys are generated at runtime — there is no key exchange protocol. Public keys are
exchanged out-of-band (pasted between terminals):

- The receiver generates an ElGamal keypair and prints its public key for the sender.
- The sender generates an FSS keypair and prints its public key for the receiver.
- All group parameters `(p, q, g)` for both ElGamal and FSS are fixed/hardcoded — only
  the public keys above need to be exchanged.

**On the receiver machine first:**
```bash
./receiver
# Prints ElGamal public key (paste into sender), then waits for the
# sender's FSS public key to be pasted in before it starts listening.
```

**On the sender machine:**
```bash
./sender <receiver_elgamal_pkey_hex> <receiver_ip>
# Prints its FSS public key (paste into receiver), waits for Enter,
# then begins transmission.
```

## Security Notes and Further Details
This project has no connection in any way to the Double-Ratchet Algorithm (previously
known as the Axolotl Algorithm).

While many of the v0.1 security holes were fixed, some remain (see PROTOCOL.md):
- No authenticated key exchange — public keys are exchanged out-of-band and not
  verified; vulnerable to MITM if not checked independently.
- No perfect forward secrecy.
- GNU MP is not designed to resist all side-channel attacks.
- Each Limb is independently authenticated (encrypt-then-sign); a tampered Limb is
  detected and never decrypted, but is otherwise reported to the caller — not
  retransmitted or alerted on.
- Loss beyond 25% per Limb is unrecoverable for that Limb.
- Experimental implementation; not audited.

Consider this a warning and a reminder: while the project follows modern security
practices where practical, it is experimental, began as a learning project, and should
not be used for any real-world application.

## Acknowledgements
Tested on the [ProLUG](https://discord.gg/23dQAQ42e) lab environment — a community-run
Linux upskilling lab. Thanks to the ProLUG community for the test infrastructure and
feedback.