class Main inherits IO
{
   main() : Object {
     let x : Int <- 7 + 1, y : Int in { y <- x.copy(); out_int(y * 2); }
   };
};
