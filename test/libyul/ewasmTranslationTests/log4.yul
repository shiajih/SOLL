// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
  log4(0x20, 0x40, 0x60, 0x80, 0xa0, 0xb0)
}
// ----
// Trace:
//   INVALID()
// Memory dump:
// Storage dump:
