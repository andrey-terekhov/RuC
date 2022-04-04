void main(){
  int c = 4, d = 6, r;
  int  m[2] = {6, 2};

  r = d % c;

  assert(r == 2, "r must be 2");

  r = m[0] % m[1];

  assert(r == 0, "r must be 0");
}