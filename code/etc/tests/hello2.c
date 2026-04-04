int data = 567;

code() {
  static int x;
  x += 1;
  return x;
}