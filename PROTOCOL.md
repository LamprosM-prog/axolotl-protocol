# Axolotl Protocol v0.2

## 1. Overview
The Axolotl Protocol implements Reed-Solomon erasure coding, ElGamal encryption, and
Fiat-Shamir/Schnorr (FSS) signatures to provide confidentiality, integrity, and
authenticity over UDP, without ARQ (Automatic Repeat Request). Once a message is sent,
it is never retransmitted. Loss correction is handled entirely by Reed-Solomon;
tampering detection is handled by FSS.

## 2. Goals and Non-Goals
**Goals:**
- Data integrity even under packet loss, without retransmission.
- Confidentiality via ElGamal encryption (4096-bit group).
- Authenticity via encrypt-then-sign (FSS/Schnorr over a 2048-bit safe prime group).
- Per-limb disposition reporting (`LimbStatus`), so the caller can distinguish loss
  from tampering.
- Simple, callable API: caller provides a socket, keys, and data; protocol handles framing.

**Non-Goals:**
- Guaranteed delivery — loss beyond 25% per Limb causes irrecoverable data loss for that Limb.
- Perfect forward secrecy (future work).
- Active defense — the protocol *detects* tampering via signature verification and
  reports it, but does not retransmit, alert third parties, or terminate any session
  (none exists to terminate). Response to a `LIMB_TAMPERED` disposition is the calling
  application's responsibility.
- Key exchange / PKI — public keys are exchanged out-of-band by the application running
  the protocol (see §8).

## 3. Terminology
- **Symbol**: A 2-byte unit over GF(2^16), the field used by the Reed-Solomon codec.
- **Axolotl Packet**: A 100-byte logical unit. The atomic piece of data sent in one UDP datagram.
- **Limb**: A 3200-byte Reed-Solomon codeword (1600 symbols). Consists of 32 Axolotl
  packets of 100 bytes each.
- **Frame**: The 1600-byte (800-symbol) data payload of a Limb before RS encoding —
  contains ciphertext, signature, and padding (see §4).
- **UDP Datagram**: The actual network packet carrying one Axolotl packet on the wire.

## 4. Limb Structure

A Frame is always exactly 1600 bytes (800 symbols):

| Field     | Bytes       | Symbols | Content                                  |
|-----------|-------------|---------|-------------------------------------------|
| c1        | 0 – 511     | 0–255   | ElGamal ciphertext component 1 (4096-bit)  |
| c2        | 512 – 1023  | 256–511 | ElGamal ciphertext component 2 (4096-bit)  |
| e         | 1024 – 1055 | 512–527 | FSS challenge (signature component)        |
| s         | 1056 – 1311 | 528–655 | FSS response (signature component)         |
| padding   | 1312 – 1599 | 656–799 | Zero-filled, reserved                      |

The Frame is RS-encoded (n=1600, k=800, GF(2^16)) into a 3200-byte Limb:

| Symbols     | Bytes        | Content        |
|-------------|--------------|-----------------|
| 0 – 799     | 0 – 1599     | Frame (data)    |
| 800 – 1599  | 1600 – 3199  | RS parity       |

- **c1** and **c2** are the two 512-byte components of an ElGamal ciphertext, produced
  by encrypting 32 bytes of raw data under a 4096-bit prime.
- **e** and **s** are the Fiat-Shamir/Schnorr signature over `c1 || c2`, computed under
  a separate 2048-bit safe-prime group (g=4).
- **padding** (288 bytes) exists purely to satisfy the RS(1600,800) parameter choice
  and carries no information.
- Raw data shorter than 32 bytes is zero-padded to exactly 32 bytes before encryption.
- Messages longer than 32 bytes are split across multiple Limbs (one Limb per 32 bytes
  of raw data).

## 5. Sender Algorithm
1. The caller generates an ElGamal keypair (for the receiver — see §8), an FSS keypair
   (for itself), and calls
   `axolotl_init(data, len, pkey, elgamal_params, fss_params, fss_skey)`.
2. Data is split into 32-byte chunks. The final chunk is zero-padded if needed.
3. Each chunk is ElGamal-encrypted → c1 (512 bytes) + c2 (512 bytes).
4. `c1 || c2` is signed with FSS -> `e` (32 bytes) + `s` (256 bytes).
5. The Frame is assembled: `c1 || c2 || e || s || padding` = 1600 bytes (800 symbols).
6. The Frame is Reed-Solomon encoded (GF(2^16), n=1600, k=800) -> 3200-byte Limb.
7. Each Limb is split into 32 Axolotl packets of 100 bytes.
8. The caller sets up a UDP socket and calls `axolotl_send(sockfd, dest, sess)`.
9. A `SessionOpen` handshake is sent first (total data length + number of Limbs).
10. Upon receiving `ACK`, all packets are sent sequentially over UDP.

## 6. Receiver Algorithm
1. The caller sets up a bound UDP socket and calls
   `axolotl_recv(sockfd, out_buf, out_len, skey, elgamal_params, fss_params, fss_pkey, &limb_statuses)`.
2. A `SessionOpen` message is received, `ACK` is sent back.
3. For each expected packet, a 500ms timeout is set.
   - Packet arrives → placed at correct position in the Limb buffer, marked received.
   - Timeout fires → position zero-filled, marked lost (known erasure).
4. For each Limb:
   a. The 3200-byte buffer is converted to 1600 GF(2^16) symbols.
   b. Reed-Solomon decodes the symbols, correcting up to 400 symbol errors (25%).
      - If decoding fails (errors exceed correction capacity) →
        `limb_statuses[l] = LIMB_LOST`. The Limb's bytes are not written to `out_buf`.
   c. The corrected symbols are converted back to the 1600-byte Frame, and `c1`, `c2`,
      `e`, `s` are extracted.
   d. `fs_verify(fss_params, fss_pkey, c1||c2, e, s)` is checked.
      - If verification fails → `limb_statuses[l] = LIMB_TAMPERED`. The ciphertext is
        **not decrypted** (verify-then-decrypt — see §10).
   e. If verification succeeds, `c1`/`c2` are ElGamal-decrypted → 32 bytes raw data,
      appended to `out_buf`, and `limb_statuses[l] = LIMB_OK`.
5. `out_len` is set from `total_data_len` in the handshake; `limb_statuses` (length
   `num_limbs`) is returned to the caller via the output parameter for inspection.

## 7. Limb Disposition (`LimbStatus`)
Each Limb resolves to exactly one of:

| Status          | Meaning                                                          |
|------------------|------------------------------------------------------------------|
| `LIMB_OK`        | RS decoded successfully and the signature verified.               |
| `LIMB_LOST`      | RS could not recover the Limb (errors exceeded the 25% budget).   |
| `LIMB_TAMPERED`  | RS decoded, but the FSS signature did not verify.                 |

Axolotl's contract ends at reporting this disposition per Limb — it makes no decisions
about retransmission, session termination, or alerting, and it guarantees that
plaintext is never produced for a `LIMB_TAMPERED` Limb. What the calling application
does with `limb_statuses` — e.g. discarding the entire reassembled message if *any*
Limb is non-`LIMB_OK` — is an application-level policy decision, outside the protocol's
scope. For security-sensitive applications (e.g. command/request processing), a
fail-closed policy (any non-OK status → reject the whole message) is strongly
recommended.

## 8. Key Exchange & Handshake
Axolotl performs **no key exchange or authentication of key material**. Per the
"keys belong to the application" philosophy, public keys are distributed out-of-band
by the application running the protocol:

- The receiver generates an ElGamal keypair and prints its public key (`pkey`) for the
  sender to paste in.
- The sender generates an FSS keypair and prints its public key (`fss_pkey.y`) for the
  receiver to paste in.
- Group parameters `(p, q, g)` for both ElGamal and FSS are **fixed, hardcoded
  constants**, identical across all instances — they are not exchanged.

**Known limitation:** this handshake is vulnerable to a man-in-the-middle attack if
the pasted public keys are not verified through an independent channel.

## 9. Parameters

| Parameter              | Value                                    |
|------------------------|-------------------------------------------|
| RS field               | GF(2^16), primitive polynomial `0x1100B`   |
| RS codeword (n)        | 1600 symbols (3200 bytes)                  |
| RS data (k)            | 800 symbols (1600 bytes)                   |
| RS parity (n-k)        | 800 symbols (1600 bytes)                   |
| Max correctable        | 400 symbols (~8 packets, 25% per Limb)     |
| Limb size              | 3200 bytes                                  |
| Frame (raw data) size  | 1600 bytes                                  |
| Raw plaintext per Limb | 32 bytes                                    |
| Axolotl packet         | 100 bytes                                   |
| Packets per Limb       | 32                                           |
| Per-packet timer       | 500ms                                       |
| ElGamal prime          | 4096-bit, hardcoded, g=2                   |
| FSS prime              | 2048-bit safe prime, hardcoded, g=4        |

## 10. Security Properties

- **ElGamal prime size**: 4096-bit.
- **FSS prime size**: 2048-bit safe prime (`p = 2q+1`), `g=4` generates the order-`q`
  subgroup.
- **Group parameters**: fixed and public for both schemes — standard practice
  (analogous to RFC-standardized DH groups). Security relies on DLP hardness within
  the group, not secrecy of `(p,q,g)`.
- **RNG**: All ephemeral values (ElGamal session keys, FSS signing nonces, ElGamal
  long-term `skey`) use `/dev/urandom`.
- **Authentication**: Encrypt-then-sign via FSS (Fiat-Shamir/Schnorr). The receiver
  verifies `fs_verify(c1||c2, e, s)` before any decryption occurs (verify-then-decrypt
  — ciphertext that fails verification is never decrypted, eliminating chosen-ciphertext
  risk from tampered Limbs).
- **Fixed-width serialization**: c1, c2, e, s are all serialized to their fixed widths
  (right-aligned, zero-padded), preserving symbol alignment for RS.
- **No key exchange authentication**: see §8.

## 11. Known Limitations & Open Questions
- Loss beyond ~8 packets (25%) per Limb causes that Limb to be reported `LIMB_LOST`;
  its 32 bytes of plaintext are not recovered.
- A `LIMB_TAMPERED` Limb's plaintext is never produced — but other Limbs in the same
  message are independently verified and may still succeed. Whether a partial message
  is usable is an application decision (see §7).
- No automated key exchange Public keys are pasted manually (see §8); vulnerable to
  MITM if not verified out-of-band.
- No session resumption. If the full transmission is lost, there is no recovery.
- No multiplexing.One message per session.
- 288 bytes of padding per Frame are unused; reserved for future fields.
- `g=2` for the ElGamal group may leak one bit of plaintext (quadratic residuosity) if
  messages aren't encoded into the prime-order subgroup. Will revise
- End-to-end network testing on real hardware (loss/corruption under genuine UDP) is
  in progress.