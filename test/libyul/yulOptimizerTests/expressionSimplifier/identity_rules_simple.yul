// RUN: %soll -lang=Yul %s
// REQUIRES: YulFull
{
    let a := mload(0)
    let b := sub(a, a)
}
// ====
// step: expressionSimplifier
// ----
// {
//     let a := mload(0)
//     let b := 0
// }
