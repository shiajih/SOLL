// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
  sstore(0, pc())
}
// ----
// Trace:
//   INVALID()
// Memory dump:
// Storage dump:
