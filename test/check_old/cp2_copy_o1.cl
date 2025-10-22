class A {
  a: Int <- 249;
  b: Int <- 614;
  c: Int <- 9;
  d: Int <- 7;
  getSum(): Int { a + b + c + d };
};

class Main inherits IO {
  io: IO <- new IO;

  main():Object {
    let
      a: A <- new A,
      copy_a: A
      in {
        copy_a <- a.copy();
        io.out_int(copy_a.getSum());
      }
  };
};
