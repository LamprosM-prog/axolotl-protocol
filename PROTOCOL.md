# Axolotl Protocol v0.1

## 1. Overview
The Axolotl Protocol implements Reed-Solomon erasure coding and ElGamal encryption to ensure
data integrity and confidentiality over UDP, without the use of ARQ (Automatic Repeat Request).
Once a message is sent, it is never retransmitted. Loss correction is handled entirely by Reed-Solomon.

## 2. Goals and Non-Goals
**Goals:**
- Data integrity even under packet loss, without retransmission.
- Confidentiality via ElGamal encryption.
- Simple, callable API: caller provides a socket and data, protocol handles the rest.

**Non-Goals:**
- Guaranteed delivery — loss beyond 25% per Limb will cause irrecoverable data loss.
- Perfect forward secrecy (future work).
- Protection against active attackers — ciphertext is malleable, no MAC present (see §7).

## 3. Terminology
- **Axolotl Packet**: A 16-byte logical unit. The atomic piece of data sent in one UDP datagram.
- **Limb**: A 256-byte Reed-Solomon codeword. Consists of 16 Axolotl packets of 16 bytes each.
- **UDP Datagram**: The actual network packet carrying one Axolotl packet on the wire.

## 4. Limb Structure
A Limb is always exactly 256 bytes:

| Packets | Bytes     | Content              |
|---------|-----------|----------------------|
| 0 – 3   | 0 – 63    | c1 (ElGamal)         |
| 4 – 7   | 64 – 127  | c2 (ElGamal)         |
| 8 – 15  | 128 – 255 | RS parity            |

- **c1** and **c2** are the two 64-byte components of an ElGamal ciphertext, produced by encrypting 32 bytes of raw data.
- **RS parity** is 128 bytes appended by Reed-Solomon encoding over GF(256).
- Raw data shorter than 32 bytes is zero-padded to exactly 32 bytes before encryption.
- Messages longer than 32 bytes are split across multiple Limbs (one Limb per 32 bytes).

## 5. Sender Algorithm
1. The caller generates an ElGamal keypair and calls `axolotl_init(data, len, pkey, params)`.
2. Data is split into 32-byte chunks. The final chunk is zero-padded if needed.
3. Each chunk is ElGamal-encrypted → c1 (64 bytes) + c2 (64 bytes) = 128 bytes ciphertext.
4. The 128-byte ciphertext is Reed-Solomon encoded → 128 parity bytes appended → 256-byte Limb.
5. Each Limb is split into 16 Axolotl packets of 16 bytes.
6. The caller sets up a UDP socket and calls `axolotl_send(sockfd, dest, sess)`.
7. A `SessionOpen` handshake is sent first (total data length + number of Limbs).
8. Upon receiving `ACK`, all packets are sent sequentially over UDP.

## 6. Receiver Algorithm
1. The caller sets up a bound UDP socket and calls `axolotl_recv(sockfd, out_buf, out_len, skey, params)`.
2. A `SessionOpen` message is received, `ACK` is sent back.
3. For each expected packet, a 500ms timeout is set.
   - Packet arrives → placed at correct position in the Limb buffer, marked received.
   - Timeout fires → position zero-filled, marked lost.
4. Once all 16 packets have been received or timed out, Reed-Solomon decodes the 256-byte buffer, correcting any errors introduced by zero-filling.
5. c1 (bytes 0–63) and c2 (bytes 64–127) are extracted and ElGamal-decrypted → 32 bytes raw data.
6. All chunks are concatenated. Trailing padding is stripped using `total_data_len` from the handshake.

## 7. Parameters

| Parameter         | Value                        |
|-------------------|------------------------------|
| RS field          | GF(256)                      |
| RS codeword (n)   | 256 symbols (bytes)          |
| RS data (k)       | 128 symbols (bytes)          |
| RS parity (n-k)   | 128 symbols (bytes)          |
| Max correctable   | 128 byte errors (~8 packets) |
| Target max loss   | 4 packets (25%) per Limb     |
| Limb size         | 256 bytes                    |
| Raw data per Limb | 32 bytes                     |
| Axolotl packet    | 16 bytes                     |
| Packets per Limb  | 16                           |
| Per-packet timer  | 500ms                        |
| ElGamal prime     | 512-bit safe prime (see §8)  |

## 8. Security Properties
Current security is intentionally minimal for the v0.1 skeleton:

- **ElGamal prime size**: 512-bit. Modern standard is ≥2048-bit. Will be updated.
- **Malleability**: ElGamal ciphertexts are malleable. An attacker can multiply c2 by a known value to produce a predictable plaintext change. No MAC present yet.
- **RNG**: Ephemeral key generation uses `/dev/urandom`. Session key generation uses `time(NULL)` — **not cryptographically secure**, will be replaced.
- **No authentication**: Receiver cannot verify sender identity or message integrity beyond RS error correction.
- **Fixed-width serialization**: c1 and c2 are serialized to exactly 64 bytes each (left-padded), to preserve RS byte alignment.

Security hardening is deferred until the protocol skeleton is complete and tested end-to-end.

## 9. Known Limitations & Open Questions
- Loss beyond ~8 packets per Limb may cause irrecoverable data loss.
- No key exchange mechanism — caller is responsible for distributing public keys out of band.
- No session resumption — if the full transmission is lost, there is no recovery.
- No multiplexing — one message per session.
- End-to-end network testing is pending.
