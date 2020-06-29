void main () 
{ 
float a, b, s = 0, s1 = 1, c; 
int i;  
//getid (a);  
//getid (b); 
a = 0; 
b = 3.1415926; 
c = (b-a)/10; 
while(abs(s - s1) > 1e-6)
{ 
s1 = s; 
s = 0; 
for (i = 0;i *c < b - a;i++) 
s += ((sin(a + i* c) + sin(a + (i + 1)*c))/2)*c;  
c = c/2; 
} 
print (s); 
}