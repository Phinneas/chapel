class C {
  var u: integer;
  var v: integer;
  var w: integer;
  var x: integer;
  var y: integer;
  var z: integer;
}

function foo() {
  var s: string = "1234567890";

  s = "";
}

var c = C();
foo();

_chpl_memtest_printMemStat();
