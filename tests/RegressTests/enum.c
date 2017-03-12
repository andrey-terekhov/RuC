enum Gender {
    MALE,
    FEMALE
};

enum Token {
    SYMBOL,         //0
    NUMBER,         //1
    EXPRESSION = 0, //0
    OPERATOR,       //1
    UNDEFINED       //2
};

enum Token2 {
    SYMBOL,             //0
    NUMBER,             //1
    EXPRESSION = 10,    //10
    OPERATOR,           //11
    UNDEFINED           //12
};

   int main()
{
   enum Gender a, b;
   a = MALE;
   b = FEMALE;
   print ("a  0");
   printid(a);
   print ("b  1");
   printid(b);

   enum Token b1, c1, d1;
   b1 = NUMBER;
   c1 = EXPRESSION;
   d1 = OPERATOR;
   print ("b1  1");
   printid(b1);
   print ("c1  0");
   printid(c1);
   print ("d1  1");
   printid(d1);

   enum Token2 c2, d2, e2;
   c2 = EXPRESSION;
   d2 = OPERATOR;
   e2 = UNDEFINED;
   print ("c2  10");
   printid(c2);
   print ("d2  11");
   printid(d2);
   print ("e2  12");
   printid(e2);

   return 0;
}