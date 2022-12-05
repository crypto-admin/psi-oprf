

# [PSI](https://eprint.iacr.org/2020/729.pdf)


### Private Set Intersection in the Internet Setting From Lightweight Oblivious PRF

By *Melissa Chase, Peihan Miao* <br>in [Crypto2020](https://crypto.iacr.org/2020/acceptedpapers.php)
[Paper available here.](https://eprint.iacr.org/2020/729.pdf)


### Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [PSI Source Code](#aby-source-code)
    - [Repository Structure](#repository-structure)
    - [Building](#building-the-aby-framework)
    - [RUN](#testing)

Features
---
PSI CM20 proposes an  efficient scheme to implement PSI, based on **OPRF**, **AES**, and **Hash**. 

This Code not based on **libOTe**, and impl the ECC operation with SMx algorithm.


This code is provided as a experimental implementation for testing purposes and should not be used in a productive environment. We cannot guarantee security and correctness.

Requirements
---

* Only compiled and tested On Ubuntu20.04.
* **Required packages:**
  * [`bazel`](https://bazel.build/start/cpp) (version >= 4.2.0)
  * [`libprotoc`](https://developers.google.com/protocol-buffers/docs/cpptutorial)(version >= 3.18.1)
  

PSI-OPRF Source Code
---
#### Repository Structure
* `src/`    - Source code.
 * `src/crypto/` - Source of SMx algorith.
 * `src/proto/` - Define protobuf
#### Building

1. Clone repository by running:
    ```
   git clone https://github.com/crypto-admin/psi-oprf.git
    ```

2. Enter the Framework directory: `cd psi-oprf`

3. Use bazel build with BUILD:
    ```
    bazel build //src:psiServer
    bazel build //src:psiClient
    ```

##### Run
```
    ./psiServer --senderSize=1024, --receiverSize=1024, --width=20, --port=50051
    ./psiClient --senderSize=1024, --receiverSize=1024, --width=20, --port=50051, --ip=127.0.0.1
```
