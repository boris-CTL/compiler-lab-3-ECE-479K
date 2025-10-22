-- Test of primitive initialization with new

class Main inherits IO
{
   x: Int <- new Int;

   main(): Object { out_int(x) };
}; 
