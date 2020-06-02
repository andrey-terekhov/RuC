float f (float x) 
{ 
    return sin (x); 
}

void main () 
{ 
    char G[50][50], H[50]; 
    int j, i, u; 
    float a; 
    for ( i = 0; i < 50; i++) 
    { 
        for (j = 0; j < 50; j++) 
            G[ i ][ j ] = ' ';                                  //заполняем экран пробелами
        a = i; 
        if ((a > -1)&&(a < 50))
        {
            u = 50-round(a)-1;
            G[ i ][ u ]='@';
        }                                //заполняем экран графиком f
    } 
    for ( i = 0; i < 50 ;i++)                            //выводим изображение        
    { 
        for ( j = 0; j < 50; j++) 
            H[ j ]=G[ j ][ i ]; 
        print(H); 
    } 
}
