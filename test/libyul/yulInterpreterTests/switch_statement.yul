// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
    switch 7
        case 7 { mstore(1, 2) }
        case 3 { mstore(6, 7) }
        default { mstore(8, 9) }
}
// ----
// Trace:
// Memory dump:
//     20: 0200000000000000000000000000000000000000000000000000000000000000
// Storage dump:
