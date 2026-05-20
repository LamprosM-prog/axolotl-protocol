# Axolotl Protocol v0.1

## 1. Overview
The Axolotl Protocol implements Reed-Solomon erasure coding and ElGamal encryption to ensure
data integrity and confidentiality over UDP, without the use of ARQ (Automatic Repeat Request).
Once a message is sent, it is never retransmitted. Loss correction is handled entirely by Reed-Solomon.

## 2. Goals and Non-Goals
**Goals:**
- Data integrity even under packet loss, without retransmission.
- Confidentiality via ElGamal encryption.
- Simplicity: no handshake, no session state, no ARQ.

**Non-Goals:**
- Guaranteed delivery (loss beyond 25% per Limb will cause irrecoverable data loss).
- Perfect forward secrecy (future work).
- Protection against active attackers (ciphertext is malleable — see §7).

## 3. Terminology
- **Axolotl Packet**: A 16-byte logical unit. The atomic piece of data sent in one UDP datagram.
- **Limb**: A 256-byte Reed-Solomon codeword. Consists of 16 Axolotl packets. Each Limb is independent and self-contained.
- **UDP Datagram**: The actual network packet carrying one Axolotl packet on the wire.

## 4. Limb Structure
A Limb is always exactly 256 bytes, split into 16 Axolotl packets of 16 bytes each:

| Packets | Bytes in Limb | Content          |
|---------|---------------|------------------|
| 0 – 3   | 0   – 63      | c1 (ElGamal)     |
| 4 – 7   | 64  – 127     | c2 (ElGamal)     |
| 8 – 15  | 128 – 255     | RS parity        |

- **c1** and **c2** are the two 64-byte components of the ElGamal ciphertext, each derived from encrypting 32 bytes of raw data.
- **RS parity** is 128 bytes appended by Reed-Solomon encoding over GF(256), giving n=256, k=128.
- Raw data shorter than 32 bytes is padded with random bytes to exactly 32 bytes before encryption.
- Messages longer than 32 bytes are split across multiple Limbs (one Limb per 32 bytes of raw data).

## 5. Sender Algorithm
1. Split the raw message into 32-byte chunks. Pad the final chunk with random bytes if needed.
2. For each chunk, encrypt using ElGamal → produces c1 (64 bytes) and c2 (64 bytes) = 128 bytes total.
3. Reed-Solomon encode the 128-byte ciphertext → appends 128 parity bytes → 256-byte Limb.
4. Split the Limb into 16 Axolotl packets of 16 bytes each.
5. Send each packet sequentially as a UDP datagram. Send all Limbs sequentially.

## 6. Receiver Algorithm
1. For each expected Axolotl packet, open a 50ms timer upon sending the previous packet.
2. If the packet arrives within 50ms, place its 16 bytes at the correct position in the Limb buffer.
3. If the timer expires, treat the packet as lost: fill its 16 positions in the Limb buffer with 0x00 (known erasures).
4. Once all 16 packets have been received or timed out, run Reed-Solomon erasure decoding on the 256-byte buffer.
   - Known erasure positions are the zero-filled slots.
   - Up to 4 lost packets (64 erasure bytes) can be corrected, since 2t ≤ 128 parity bytes.
5. Extract the recovered c1 (bytes 0–63) and c2 (bytes 64–127).
6. Decrypt using ElGamal → 32 bytes of raw data.
7. Repeat for each Limb. Concatenate all recovered chunks to reconstruct the original message.
8. Strip trailing padding from the final chunk based on known message length (TBD — see §9).

## 7. Parameters
| Parameter         | Value                        |
|-------------------|------------------------------|
| RS field          | GF(256)                      |
| RS codeword (n)   | 256 symbols (bytes)          |
| RS data (k)       | 128 symbols (bytes)          |
| RS parity (n-k)   | 128 symbols (bytes)          |
| Max erasures      | 128 bytes = 8 packets        |
| Target max loss   | 4 packets (25%) per Limb     |
| Limb size         | 256 bytes                    |
| Raw data per Limb | 32 bytes                     |
| Axolotl packet    | 16 bytes                     |
| Packets per Limb  | 16                           |
| Per-packet timer  | 50ms                         |
| ElGamal prime     | 512-bit safe prime (see §8)  |

## 8. Security Properties
Current security is intentionally minimal for the v0.1 skeleton:

- **ElGamal prime size**: 512-bit. Modern standard is ≥2048-bit. This will be updated.
- **Malleability**: ElGamal ciphertexts are malleable. An attacker can multiply c2 by a known value to produce a predictable change in plaintext. No authentication or MAC is present yet.
- **RNG**: Key generation and encryption use `time(NULL)` as RNG seed — **not cryptographically secure**. Will be replaced with `/dev/urandom`.
- **Fixed-width serialization**: c1 and c2 must be serialized to exactly 64 bytes each (left-padded with zeros if needed), otherwise RS byte alignment breaks.

Security hardening is deferred until the protocol skeleton is complete and tested.

## 9. Known Limitations & Open Questions
- Loss beyond 4 packets (25%) per Limb causes irrecoverable data loss for that Limb.
- No mechanism yet for the receiver to know the original message length (needed to strip padding from final Limb). To be defined.
- No authentication — receiver cannot verify sender identity or message integrity beyond RS error correction.
- Single-send only: if the entire transmission is lost (e.g. network down), there is no recovery.
