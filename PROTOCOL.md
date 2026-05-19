# Axolotl Protocol v0.1

## 1. Overview
The Axolotl Protocol aims to implement Reed-Solomon and Elgamal encryption in order to ensure
data integrity without the use of ARQ.

## 2. Goals and Non-Goals
- Total data integrity even if connection is terminated after intial send.
- Security
## 3. Packet Format
Data will be split into Limbs. Each Limb will hold 16 packets each with 16 bytes each, for a total of 256 bytes. (Each Limb is a Reed-Solomon codeword).
This happens so that the receiver will know how to structure the data. This is required for Reed-Solomon
to work correctly. If a packet takes too long to send we treat it as lost and fill its positions with 0.
This allows us to correct up to 25% loss per Limb (4 lost packets). UDP loss is around 5% under normal conditions.
With WiFi and other conditions we expect it to raise to 15%, way below our threshold.

Further testing will be done.

## 4. Sender Algorithm
1. Data will be split to Limbs. Each Limb will hold 32 bytes of raw data.
2. Encrypt data, using ElGamal. ElGamal encrypts a 32 byte data to 2 ciphertexts the size of 64 bytes. So each Limb will have 128 bytes of data
3. Encode using Reed-Solomon. This appends 128 bytes. So the total is 256 bytes and the Limb is ready to be sent.

## 5. Receiver Algorithm
1. Receiver knows how much data it expects per Limb (32 bytes raw, 256 encrypted and encoded)
2. Packages are numbered. A timer will be set and if it exceeds a certain amount of ms we will consider the package lost and fill its positions with 0.
3. Receiver will decode the package. Correcting any errors and leaving only the encrypted ciphertexts.
4. Using ElGamal the ciphers will be decrypted and the data will be re-constructred.

## 6. Parameters
WIP

## 7. Security Properties
Currently the security is not up to par with modern security standards.
ElGamal commonly uses >2056 prime number but right now it uses a 512 byte prime number.
The cipher text is mallable.

Security **will** be updated once the skeleton of the protocol is formatted.
## 8. Known Limitations
- If more than 25% loss is experienced, data loss is to be expected.
