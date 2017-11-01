void main ()
{
      char M[50];
      char Stack [40][2] ;
    char c;
    int i,k=0,p=-1,w;
    while (c != ';')
    {
    	getid (c); 
	     
	    w=15;
	if (  ('0' <= c && c <= '9') || ( 'a' <= c && c <= 'z') ||('A' <= c && c <= 'Z')  )
               {
                    M[k++]=c;
               
                }
            else
            {
	       if (c=='*' || c=='/' ) 
	           w = 8;
	       if (c=='+' || c=='-' ) 
	           w =7; 
	       if (c=='(' || c==')' ) 
	           w=6;
	        if (p != -1)
	           while ( p != -1 && Stack[p][1]>=w) 
	          	     {
	                	 M[k++]=Stack [p--][0] ;
		                 if (p==-1)
		                 break;	
	                  }
 	       
	          Stack [++p][0]=c;
	          Stack [p][1]=w;
	          if (c==')')
	          {
	          p--;
	         M[--k]=' ';
	         }
            };;
 
 // print (M);  	     
         }
        //  print (M);
          p--;  
       while (p!=-1)
      	 M[k++]=Stack [p--][0];
       print (M);       
 
}