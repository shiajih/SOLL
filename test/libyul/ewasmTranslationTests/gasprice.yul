// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
  sstore(0, gasprice())
}
// ----
// Trace:
// Memory dump:
//     20: 0000000000000000000000006666666600000000000000000000000000000000
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000000: 0000000000000000000000006666666600000000000000000000000000000000
