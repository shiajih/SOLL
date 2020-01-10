// RUN: %soll -lang=Yul %s
// REQUIRES: yulType
{
    let a:u256
    function f() { let b:u256 }
    let c:u256
    function g() { let d:u256 }
    let e:u256
}
// ====
// step: mainFunction
// yul: true
// ----
// {
//     function main()
//     {
//         let a:u256
//         let c:u256
//         let e:u256
//     }
//     function f()
//     { let b:u256 }
//     function g()
//     { let d:u256 }
// }
