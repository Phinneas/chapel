class C {
  var D: domain(1);
  var A: [D] real;

  def foo() {
    writeln("D is: ", D);
    writeln("A is: ", A);
    D = [1..10];
    writeln("D is: ", D);
    writeln("A is: ", A);
    A(4) = 1.2;
    writeln("D is: ", D);
    writeln("A is: ", A);
  }
}

var myC = new C();
myC.foo();
