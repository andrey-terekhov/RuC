int glob = 5;

struct B 
{
  int x;
};

struct A 
{
  struct B inner[glob];
};


int main() 
{
  struct A a1[5];
  glob = 6;
  {
     struct A a2;
     a2.inner[4].x = 3;

     assert(a2.inner[4].x * 2 == 6, "a2.inner[4].x * 2 must be 6");
  }
  return 0;
}
