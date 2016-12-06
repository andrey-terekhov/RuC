void main(){
 int* y;
 int z;
 z= &y;
 y=4;
 printid(z);
 y=-4;
 printid(z);
}
