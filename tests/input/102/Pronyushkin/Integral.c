float f (float x)
{
	return sin (x); 
}
void main ()
{
float a,b,d,S, S_1; 
int i,k = 0; 
S = 1;
S_1 = 0;  
getid (d);
getid (a); 
getid (b);
while (abs (S - S_1) > 1e-6)
{	
	S_1 = S; 
	S = 0;
	for (i = 0; i* d < b - a; i++)
		S += ((f(a + i*d) + f(a + (i + 1)*d))/2)* d;
	d = d/2;
	printid (S_1);
	k ++; 
}
printid (S);
printid (d); 
printid (k);
}