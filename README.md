

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
##### Update
```
  20230206跑通了大数据100W的正确性，问题在于grpc超时，将timeout设置为1000s后解决；
  20230207提升新能，使用了AES的大表实现，如果用AESNI/SM4NI应该会更快；
  目前的性能比libOTE版本慢5倍，还有太多优化空间;
  20230209: 使用了AESNI/BLAKE3; AESNI带来的性能提升显著，100w*100w 在19s内.
  优化随机数生成函数，目前程序正确性通过.

TODO：
  性能仍然比libOTe版本慢1倍，问题在哪里？
```