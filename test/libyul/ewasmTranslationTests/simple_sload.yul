// RUN: %soll -lang=Yul %s
{
  sstore(1, 7)
  sstore(2, sub(0, 1))
  sstore(3, sload(1))
  sstore(4, sload(2))
}
// ----
// Trace:
// Memory dump:
//      0: 0000000000000000000000000000000000000000000000000000000000000004
//     20: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000001: 0000000000000000000000000000000000000000000000000000000000000007
//   0000000000000000000000000000000000000000000000000000000000000002: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
//   0000000000000000000000000000000000000000000000000000000000000003: 0000000000000000000000000000000000000000000000000000000000000007
//   0000000000000000000000000000000000000000000000000000000000000004: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
