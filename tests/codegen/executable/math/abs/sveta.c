void main()
{
 float xx = abs(4.);

 assert(xx == 4.000000, "xx must be 4.000000");

 xx = abs(-xx);
 
 assert(xx == 4.000000, "xx must be 4.000000");
}