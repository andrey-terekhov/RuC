struct point_t {
    int x;
    int y;
} B;

   int main()
{
   struct point_t A;
   float distance;
 
   A.x = 10;
   A.y = 20;
 
   distance = sqrt((float) (A.x*A.x + A.y*A.y));

   print ("distance  22.360680");
   printid(distance);

   B.x = 11;
   int bx = B.x;

   print ("bx  11");
   printid(bx);

   struct {
        int x;
        int y;
    } C;

   C.x = 12;
   C.y=14;
   distance = sqrt((float) (C.x*C.x + C.y*C.y));

   print ("distance  18.439089");
   printid(distance);

   return 0;
}