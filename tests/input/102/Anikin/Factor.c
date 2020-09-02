
int f (int x)
 {
 if (x)
 return x*f(x-1);
 return 1;
 }
 void main ()
 {
    int n;
   getid(n);
   printid(n);
   print( f(n) );
}
 
 