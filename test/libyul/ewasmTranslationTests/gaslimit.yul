// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
  sstore(0, gaslimit())
}
// ----
// Trace:
// Memory dump:
//     20: 00000000000000000000000000000000000000000000000000000000003d0900
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000000: 00000000000000000000000000000000000000000000000000000000003d0900
