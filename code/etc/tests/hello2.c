int data = 567;

int code() {
  static int x;
  x += 1;
  return x;
}