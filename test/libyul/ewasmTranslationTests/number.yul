// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
  sstore(0, number())
}
// ----
// Trace:
// Memory dump:
//     20: 0000000000000000000000000000000000000000000000000000000000000400
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000000: 0000000000000000000000000000000000000000000000000000000000000400
