var s1: string;
var flag1: sync bool;
var done: sync bool;

def main() {
  s1 = "foo";
  on Locales(1) do begin {
    const tmp = flag1;
    _debugWriteln("s1 is: ", s1);
    done = true;
  }
  s1 = "boohoo";
  flag1 = true;
  done;

  var s2: string;
  var flag2: sync bool;

  s2 = "foo";
  on Locales(1) do begin {
    const tmp = flag2;
    _debugWriteln("s2 is: ", s2);
  }
  s2 = "boohoo";
  flag2 = true;
}
