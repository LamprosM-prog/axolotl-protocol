# Changelog

## v0.2
### Reed-Solomon
- Rewrote RS codec from GF(256) to GF(2^16) (primitive polynomial `0x1100B`).
- RS parameters changed from (n=256, k=128, t=4 packets) to (n=1600, k=800, t≈8 packets),
  preserving the 25%-per-Limb correction ratio.
- `forney()` now returns the corrected codeword rather than mutating in place.

### Framing
- Packet size: 16 bytes → 100 bytes. Packets per Limb: 16 → 32. Limb size: 256 bytes → 3200 bytes.
- New Frame layout: `c1 (512B) || c2 (512B) || e (32B) || s (256B) || padding (288B)` = 1600B.

### Cryptography
- ElGamal prime: 512-bit → 4096-bit.
- ElGamal long-term key generation: `time(NULL)` seeding → `/dev/urandom`.
- Added Fiat-Shamir/Schnorr (FSS) signatures: encrypt-then-sign over `c1||c2`,
  2048-bit safe-prime group, `g=4`.
- Verify-then-decrypt: receiver checks `fs_verify` before any ElGamal decryption.

### API
- `axolotl_init` now takes `fss_params` and `fss_skey` in addition to ElGamal params.
- `axolotl_recv` now takes `fss_params`, `fss_pkey`, and an output `LimbStatus **limb_statuses`.
- New `LimbStatus` enum: `LIMB_OK`, `LIMB_LOST`, `LIMB_TAMPERED` — per-Limb disposition
  reporting, decoupled from any retransmission/session logic.

### Key Exchange
- FSS public key (`y`) now also exchanged out-of-band, sender → receiver (in addition
  to the existing ElGamal pkey, receiver → sender).
- Confirmed both ElGamal and FSS group parameters `(p,q,g)` are fixed/hardcoded —
  no parameter negotiation needed, only public keys are exchanged.

### Testing
- Added `sender_raw`/`receiver_raw`: handshake + 32×100B packet framing with no RS/crypto,
  for measuring real UDP loss as a baseline against the 25% correction budget.

## v0.1
- Initial skeleton: GF(256) RS(256,128), 512-bit ElGamal, 16×16B packets, no authentication.