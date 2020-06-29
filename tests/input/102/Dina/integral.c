float a, b, s, s1, d, e = 1e-6;
int i, k, t, n;
void main()
{
    getid(a);
    getid(b);
    n = 10;
    s = 0; 
    s1 = 0;
    
    do 
    {
        d = (b-a)/n;
            for (i = 0; i < n; i++) 
            {
                s = s+((sin(a+i*d)+sin(a+(i+1)*d))/2)*d;
            }
         if (abs(s-s1) > e) 
         { 
         s1 = s;
         s = 0;
         n = n*2;
         }  
     }
    while (abs(s-s1)>e);
    printid(s);
    printid(n);
}