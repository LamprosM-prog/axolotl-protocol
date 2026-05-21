# Axolotl Protocol

A UDP-based messaging protocol combining Reed-Solomon erasure coding and ElGamal encryption to guarantee data integrity and confidentiality without retransmission (no ARQ).

Once a message is sent, it is never resent. Loss correction is handled entirely by Reed-Solomon.

## How It Works

Raw data is split into 32-byte chunks. Each chunk is ElGamal-encrypted into 128 bytes (c1 + c2, 64 bytes each), then Reed-Solomon encoded into a 256-byte codeword called a **Limb**. Each Limb is split into 16 packets of 16 bytes and sent sequentially over UDP.

On the receiver side, if a packet doesn't arrive within the timeout window it is treated as lost and its position is zero-filled. Reed-Solomon corrects up to 4 lost packets (25%) per Limb. The recovered ciphertext is then ElGamal-decrypted to recover the original data.

```
raw data
  → split into 32-byte chunks
  → ElGamal encrypt → c1 (64 bytes) + c2 (64 bytes)
  → RS encode → 256-byte Limb
  → split into 16 × 16-byte packets
  → send over UDP
```

## Parameters

| Parameter        | Value                        |
|------------------|------------------------------|
| Packet size      | 16 bytes                     |
| Packets per Limb | 16                           |
| Limb size        | 256 bytes                    |
| Raw data / Limb  | 32 bytes                     |
| RS field         | GF(256)                      |
| RS codeword (n)  | 256 symbols                  |
| RS data (k)      | 128 symbols                  |
| RS parity (n-k)  | 128 symbols                  |
| Max loss / Limb  | 4 packets (25%)              |
| ElGamal prime    | 512-bit (see Security)       |

## Project Structure

```
src/
  axolotl/      ← protocol API (axolotl.h / axolotl.c)
  elgamal/      ← ElGamal encryption
  rs/           ← Reed-Solomon over GF(256)
tests/
  sender.c      ← test sender harness
  receiver.c    ← test receiver harness
```

## Dependencies

- `gcc`
- `libgmp` — for ElGamal big integer arithmetic

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

Produces `sender` and `receiver` binaries at the project root.

## Usage

Keys are generated at runtime. The receiver generates a keypair and prints its public key. The sender takes that public key as a command-line argument.

**On the receiver machine first:**
```bash
./receiver
# prints public key, then waits for connection
```

**On the sender machine — paste the printed public key:**
```bash
./sender <pkey_hex>
```

## Security Notes

Current security is intentionally minimal for the v0.1 skeleton:

- **Prime size**: 512-bit. Modern standard is ≥2048-bit. Will be updated.
- **Malleability**: ElGamal ciphertexts are malleable. No MAC or authentication present yet.
- **RNG**: Key generation uses `/dev/urandom`. Encryption ephemeral key also uses `/dev/urandom`.

Security hardening is deferred until the protocol skeleton is complete and tested. See `PROTOCOL.md` for full specification.