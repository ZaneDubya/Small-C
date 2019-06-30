// testing struct definitions:
struct teststruct {
    int inttest, *intptrtest, fourints[4];
    char onechar, fivehundredchars[500];
    int averylongnameforthisvar;
    char wherearewenow;
};

// Define a type point as a struct with integer members x, y
/*struct point {
   int    x, y;
};*/

/*int ga[3] = { 1, 2, 3 };
point gx;*/

main() {
    // Define a variable q of type point. Members are uninitialized.
    point q, p;
    // Define a variable p of type point, and initialize all its members inline!
    // point p = { 1, 3 };

    // Assign the value of p to q, copies the member values from p into q.
    //  q = p;

    // Change the member x of q to have the value of 3
    // q.x = 3;

    // Demonstrate we have a copy and that they are now different.
    //  if (p.x != q.x) printf("The members are not equal! %d != %d", p.x, q.x);

    // Define a variable r of type point. Members are uninitialized.
    //   point r;

    // Assign values using compound literal (ISO C99/supported by GCC > 2.95)
    //  r = (point) { 1, 2 };

    return 1;
}
