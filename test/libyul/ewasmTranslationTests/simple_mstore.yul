// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
  mstore(0x20, 0x0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20)
}
// ----
// Trace:
// Memory dump:
//     60: 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20
// Storage dump: