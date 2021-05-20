//  reutovr
//
//  Created by Andrey Terekhov on 12/5/12.
//  Copyright (c) 2012 Andrey Terekhov. All rights reserved.
//

/*
#include <stdio.h>
#include <time.h>
#include <math.h>
*/
#define m 128
#define n 128
#define mn m*n
#define M 512
#define N 512

int main()
{
  /*
    FILE *input;
    
    clock_t start_time, end_time;
    start_time = clock();
    */
    int maxk, maxl;
	float Fr, maxFr, ar[m][n];
    char nxt, z[m][N];    
    int s[m][N], s2[m][N], z2[m][N];    
    float zvr, zzr, valr, sqrzzr;    
    int i, ii, iii, i4, jj, k, l, kk, ll, flagend, calca;
    k=0; l=0; i=0; flagend=0; calca=1;    
    maxFr=-1000000;
    
//    input = fopen("/Users/ant/Desktop/Reutov/Reutov/frame3.bmp", "r");
	for (ii=0; ii<1078; ++ii)
  //      fscanf(input, "%c", &nxt);
	;
    while (flagend == 0)
    {        
        //fscanf(input, "%c", &nxt);
        z[i][l]=nxt;        
        z2[i][l]=nxt*nxt;        
        if (l == 0)
        {
            s[i][l]=nxt;  s2[i][l]=z2[i][l];
        }
        else
        {
            s[i][l]=s[i][l-1] - ((l >= n) ? z[i][l-n] : 0) + nxt;
            
            s2[i][l]=s2[i][l-1] - ((l >= n) ? z2[i][l-n] : 0) + z2[i][l];  // s 12 bit, s2 20
        }        
        if (k >= m-1 && l >= n-1)
        {
            zvr=s[0][l]; zzr=s2[0][l];
            
            for (ii=1; ii<m; ++ii)
            {
                zvr+=s[ii][l];  zzr+=s2[ii][l];
            }            
            zvr/=mn;  zzr/=mn; zzr-=zvr*zvr;            
            if ((zzr > 0 ? zzr : -zzr) < 0.001) zzr=1;            
            sqrzzr = sqrt(zzr);            
            kk=k-m+1; ll=l-n+1; Fr=0;            
            iii=(i+1 == m) ? 0 : i+1;
            
            for (ii=0; ii<m; ++ii)
            {
                i4=iii + ii;                
                i4=(i4 >= m) ? i4-m : i4;                
                for (jj=0; jj<n; ++jj)
                {
                    valr=(z[i4][ll+jj]-zvr)/zzr;                    
                    if ( calca == 1)
                        ar[ii][jj]=valr;
                    else
                        Fr += ar[ii][jj] * valr;
                    
                }                
            }            
            if (calca == 0 && Fr > maxFr)
            {
                maxFr=Fr; maxk=kk; maxl=ll;
            }           
        }        
        if (calca == 1 && k == m-1 && l == n-1)
        {
            calca=0; k=0; l=0; i=0;
            
 //           fclose(input);
//            input = fopen("/Users/ant/Desktop/Reutov/Reutov/image3.bmp", "r");
            for (ii=0; ii<1078; ++ii)
			;//fscanf(input, "%c", &nxt);
            
        }        
        else if (++l == N && k == M-1)                
                flagend=1;        
             else if (l == (( calca == 1) ? n : N))
             {
                 ++k; l=0;                
                 if (++i == m)                    
                     i=0;
             }
    };
 //   fclose(input);
    printf("maxk =%i maxl=%i maxF=%f\n", maxk, maxl, maxFr);
    
//    end_time = clock();
 //   clock_t diffTime = end_time - start_time;
//    printf("%li\n", diffTime);
    
    return 0;
}


