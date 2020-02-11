int glob = 5;

struct B {
  int x;
};

struct A {
  struct B inner[glob];
};


int main() {
  struct A a1[5];
  glob = 6;
  {
     struct A a2;
     a2.inner[4].x = 3;
     print(a2.inner[4].x * 2);
  }
  return 0;
}
