
 float p (int a, int b)
{ int i;
  float  t=1;
  
	 for(i=0;i<b;i++)
 		t*=a;
              for(i=0; i>b; i--)
 		t/=a;
  return t;
}

void main ()
{
  int n=0,o=0, m=1;
  float x=1;
  char a;
  getid (a);
  //n+=a-'0';
 //printid (n);
  if (a == '-')
    {
	  x=-1;
	  getid (a);
     }
  if (( a <= '9' )&&(a>='0')) 
 	o=1;   
  while (( a <= '9' )&&(a>='0'))
  {
  	n=n*10+a-'0';
	m=0;
	getid (a);
   }
   if(o=1)
     {	  
 	 if (a==' ')
 	   {
 	 	  print ("целое число: ");	
		   if (x==-1)
	               n*=-1;	    
		  printid(n);
	    }
 	 if((a=='.')||(a==','))
                {										
                           getid (a);
	 	while (( a <= '9' )&&(a>='0'))
	              {
  			n=n*10+a-'0';
			  m++;
			getid (a);
		}
		if ((a==' ')&&(m>0))
		  {
 	 	  print ("вещественное число в нормальной записи: ");
		  x*=n;
		  x/=p(10, m);	
		  printid (x) ;
	                }
	             else
	             print ("это не число ");	
	                
	      }        
            	 if (a=='e')
	   {
	       getid (a);
	       if (a == '-')
                      {
		  o=-1;
		  getid (a);
                       }
	       while (( a <= '9' )&&(a>='0'))
	         {
  		m=m*10+a-'0';
		getid (a);
	         }
            	      
                     m*=o;	
	        if (a==' ')
	          {
 	 	  print ("вещественное число в экспонециальной записи: ");
		  x*=p(n,m);	
		  printid(x);
	          }	
                     else
	            	   print ("это не число "); 
			                        
	    }
	  if ((a!=' ')&&(a!='e')&&(a!='.')&&(a!=','))
	            		 print ("это не число "); 
  
     }
     if(o=0)
	            		 print ("это не число ");
   }