class InParallel {
  param a: int;
  param b: real;

  proc init(param x: int, param y: real) {
    cobegin { // Uh oh, fields can't be initialized in parallel!
      a = x;
      b = y;
    }
    super.init();
  }
}

proc main() {
  var c: InParallel(1, 2.0) = new InParallel(1, 2.0);
  writeln(c.type: string);
  delete c;
}
