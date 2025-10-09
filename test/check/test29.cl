class A {
    a: Int <- 1;
};

class Main {
  a: Int;
  b: Int;
  c: A <- new A;
  d: A <- new A;
  main(): Int {
        { if a = b then 100 else 10 fi +
          if c = d then 0 else 1 fi;
        }
  };
};
