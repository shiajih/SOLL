// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
  sstore(0, lt(1, 0))
  sstore(1, lt(0, 1))
  sstore(2, lt(0, 0))
  sstore(3, lt(not(0), 0xffffffffffffffff))
  sstore(4, lt(0xffffffffffffffff, not(0)))
  sstore(5, lt(not(0), not(0)))
  sstore(6, lt(not(0), 0xffffffffffffffff0000000000000000))
  sstore(7, lt(0xffffffffffffffff0000000000000000, not(0)))
  sstore(8, lt(0xffffffffffffffff0000000000000000, 0xffffffffffffffff0000000000000000))
  sstore(9, lt(not(0), 0xffffffffffffffff0000000000000000ffffffffffffffff))
  sstore(10, lt(0xffffffffffffffff0000000000000000ffffffffffffffff, not(0)))
  sstore(11, lt(0xffffffffffffffff0000000000000000ffffffffffffffff, 0xffffffffffffffff0000000000000000ffffffffffffffff))
  sstore(12, lt(not(0), 0xffffffffffffffff0000000000000000ffffffffffffffff0000000000000000))
  sstore(13, lt(0xffffffffffffffff0000000000000000ffffffffffffffff0000000000000000, not(0)))
  sstore(14, lt(
    0xffffffffffffffff0000000000000000ffffffffffffffff0000000000000000,
    0xffffffffffffffff0000000000000000ffffffffffffffff0000000000000000)
  )
  sstore(15, lt(
      0xffffffffffffffff0000000000000000ffffffffffffffff0000000000000000,
      0xffffffffffffffff0000000000000000ffffffffffffffff0000000000000001)
  )
}
// ----
// Trace:
// Memory dump:
//      0: 000000000000000000000000000000000000000000000000000000000000000f
//     20: 0000000000000000000000000000000000000000000000000000000000000001
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000001: 0000000000000000000000000000000000000000000000000000000000000001
//   0000000000000000000000000000000000000000000000000000000000000004: 0000000000000000000000000000000000000000000000000000000000000001
//   0000000000000000000000000000000000000000000000000000000000000007: 0000000000000000000000000000000000000000000000000000000000000001
//   000000000000000000000000000000000000000000000000000000000000000a: 0000000000000000000000000000000000000000000000000000000000000001
//   000000000000000000000000000000000000000000000000000000000000000d: 0000000000000000000000000000000000000000000000000000000000000001
//   000000000000000000000000000000000000000000000000000000000000000f: 0000000000000000000000000000000000000000000000000000000000000001
