// locals.c -- Test program for local variable initialization in Small-C 2.2.
// Tests: scalar init (int, char, pointer), array init, partial array init,
//        expression initializers, function-call initializers, multiple
//        initialized locals in one declaration, mixed init and non-init,
//        char array from string, struct init, nested struct init,
//        struct array init.
//
// Each test prints PASS or FAIL with a description.
// At the end, a summary of passed/failed/total is printed.

#include "../../smallc22/stdio.h"

int passed, failed;

struct point {
    int x, y;
};

struct rect {
    struct point topLeft, botRight;
};

struct mixed {
    char c;
    int  i;
};

// ============================================================================
// Helper functions
// ============================================================================

void check(char *desc, int cond) {
    if (cond) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s\n", desc);
        printf("  Press Enter to continue...");
        getchar();
        failed++;
    }
}

int add(int a, int b) {
    return a + b;
}

int seven() {
    return 7;
}

int double_it(int x) {
    return x + x;
}

// ============================================================================
// Main test program
// ============================================================================

int main() {
    passed = 0;
    failed = 0;
    printf("=== Local Variable Initialization Tests ===\n");

    // -------------------------------------------------------
    printf("\n--- 1. Int local with constant init ---\n");
    // -------------------------------------------------------
    {
        int x = 42;
        check("int x = 42", x == 42);
    }

    // -------------------------------------------------------
    printf("\n--- 2. Int local with expression init ---\n");
    // -------------------------------------------------------
    {
        int x = 2 + 3;
        check("int x = 2 + 3", x == 5);
    }

    // -------------------------------------------------------
    printf("\n--- 3. Int local with variable-based init ---\n");
    // -------------------------------------------------------
    {
        int x = 10;
        int y = x + 1;
        check("x == 10", x == 10);
        check("y = x + 1 == 11", y == 11);
    }

    // -------------------------------------------------------
    printf("\n--- 4. Char local with constant init ---\n");
    // -------------------------------------------------------
    {
        char c = 'A';
        check("char c = 'A'", c == 65);
    }

    // -------------------------------------------------------
    printf("\n--- 5. Pointer local init ---\n");
    // -------------------------------------------------------
    {
        int val = 99;
        int *p = &val;
        check("*p == 99", *p == 99);
    }

    // -------------------------------------------------------
    printf("\n--- 6. Int array full init ---\n");
    // -------------------------------------------------------
    {
        int arr[3] = {10, 20, 30};
        check("arr[0] == 10", arr[0] == 10);
        check("arr[1] == 20", arr[1] == 20);
        check("arr[2] == 30", arr[2] == 30);
    }

    // -------------------------------------------------------
    printf("\n--- 7. Int array partial init (rest zero) ---\n");
    // -------------------------------------------------------
    {
        int arr[5] = {1, 2};
        check("arr[0] == 1", arr[0] == 1);
        check("arr[1] == 2", arr[1] == 2);
        check("arr[2] == 0", arr[2] == 0);
        check("arr[3] == 0", arr[3] == 0);
        check("arr[4] == 0", arr[4] == 0);
    }

    // -------------------------------------------------------
    printf("\n--- 8. Multiple initialized locals in one decl ---\n");
    // -------------------------------------------------------
    {
        int a = 1, b = 2, c = 3;
        check("a == 1", a == 1);
        check("b == 2", b == 2);
        check("c == 3", c == 3);
    }

    // -------------------------------------------------------
    printf("\n--- 9. Init after uninit local ---\n");
    // -------------------------------------------------------
    {
        int x;
        int y = 5;
        x = 10;
        check("x == 10", x == 10);
        check("y == 5", y == 5);
    }

    // -------------------------------------------------------
    printf("\n--- 10. Function call in initializer ---\n");
    // -------------------------------------------------------
    {
        int x = seven();
        check("int x = seven()", x == 7);
    }

    // -------------------------------------------------------
    printf("\n--- 11. Complex expression initializer ---\n");
    // -------------------------------------------------------
    {
        int x = 3;
        int y = add(x, 4) + 1;
        check("y = add(3,4)+1 == 8", y == 8);
    }

    // -------------------------------------------------------
    printf("\n--- 12. Char array full init ---\n");
    // -------------------------------------------------------
    {
        char arr[3] = {10, 20, 30};
        check("char arr[0] == 10", arr[0] == 10);
        check("char arr[1] == 20", arr[1] == 20);
        check("char arr[2] == 30", arr[2] == 30);
    }

    // -------------------------------------------------------
    printf("\n--- 13. Init with unary expressions ---\n");
    // -------------------------------------------------------
    {
        int x = -5;
        int y = ~0;
        check("int x = -5", x == -5);
        check("int y = ~0 == -1", y == -1);
    }

    // -------------------------------------------------------
    printf("\n--- 14. Init with multiply/divide ---\n");
    // -------------------------------------------------------
    {
        int x = 6 * 7;
        int y = 100 / 5;
        check("6 * 7 == 42", x == 42);
        check("100 / 5 == 20", y == 20);
    }

    // -------------------------------------------------------
    printf("\n--- 15. Array init with expressions ---\n");
    // -------------------------------------------------------
    {
        int x = 5;
        int arr[3] = {x, x + 1, x + 2};
        check("arr[0] == 5", arr[0] == 5);
        check("arr[1] == 6", arr[1] == 6);
        check("arr[2] == 7", arr[2] == 7);
    }

    // -------------------------------------------------------
    printf("\n--- 16. Array init with function calls ---\n");
    // -------------------------------------------------------
    {
        int arr[3] = {seven(), add(1, 2), double_it(4)};
        check("arr[0] == 7", arr[0] == 7);
        check("arr[1] == 3", arr[1] == 3);
        check("arr[2] == 8", arr[2] == 8);
    }

    // -------------------------------------------------------
    printf("\n--- 17. Multiple arrays in sequence ---\n");
    // -------------------------------------------------------
    {
        int a[2] = {1, 2};
        int b[2] = {3, 4};
        check("a[0] == 1", a[0] == 1);
        check("a[1] == 2", a[1] == 2);
        check("b[0] == 3", b[0] == 3);
        check("b[1] == 4", b[1] == 4);
    }

    // -------------------------------------------------------
    printf("\n--- 18. Pointer init to array ---\n");
    // -------------------------------------------------------
    {
        int arr[3] = {100, 200, 300};
        int *p = arr;
        check("*p == 100", *p == 100);
    }

    // -------------------------------------------------------
    printf("\n--- 19. Mixed init and computation ---\n");
    // -------------------------------------------------------
    {
        int x = 10;
        int y = 20;
        int z = x + y;
        check("z = x + y == 30", z == 30);
        z = z + 5;
        check("z + 5 == 35", z == 35);
    }

    // -------------------------------------------------------
    printf("\n--- 20. Nested block init ---\n");
    // -------------------------------------------------------
    {
        int x = 1;
        {
            int x = 2;
            check("inner x == 2", x == 2);
        }
        check("outer x == 1", x == 1);
    }

    // -------------------------------------------------------
    printf("\n--- 21. Char array from string (explicit size) ---\n");
    // -------------------------------------------------------
    {
        char s[6] = "hello";
        check("s[0] == 'h'", s[0] == 'h');
        check("s[1] == 'e'", s[1] == 'e');
        check("s[4] == 'o'", s[4] == 'o');
        check("s[5] == 0 (null)", s[5] == 0);
    }

    // -------------------------------------------------------
    printf("\n--- 22. Char array from string (inferred size) ---\n");
    // -------------------------------------------------------
    {
        char s[] = "abc";
        check("s[0] == 'a'", s[0] == 'a');
        check("s[1] == 'b'", s[1] == 'b');
        check("s[2] == 'c'", s[2] == 'c');
        check("s[3] == 0 (null)", s[3] == 0);
    }

    // -------------------------------------------------------
    printf("\n--- 23. Char array larger than string ---\n");
    // -------------------------------------------------------
    {
        char s[8] = "hi";
        check("s[0] == 'h'", s[0] == 'h');
        check("s[1] == 'i'", s[1] == 'i');
        check("s[2] == 0 (null)", s[2] == 0);
        check("s[3] == 0 (zero-fill)", s[3] == 0);
        check("s[7] == 0 (zero-fill)", s[7] == 0);
    }

    // -------------------------------------------------------
    printf("\n--- 24. Struct local init ---\n");
    // -------------------------------------------------------
    {
        struct point p = {10, 20};
        check("p.x == 10", p.x == 10);
        check("p.y == 20", p.y == 20);
    }

    // -------------------------------------------------------
    printf("\n--- 25. Struct init with expressions ---\n");
    // -------------------------------------------------------
    {
        int a = 3;
        struct point p = {a + 1, a * 2};
        check("p.x == 4", p.x == 4);
        check("p.y == 6", p.y == 6);
    }

    // -------------------------------------------------------
    printf("\n--- 26. Nested struct init ---\n");
    // -------------------------------------------------------
    {
        struct rect r = {{1, 2}, {3, 4}};
        check("r.topLeft.x == 1", r.topLeft.x == 1);
        check("r.topLeft.y == 2", r.topLeft.y == 2);
        check("r.botRight.x == 3", r.botRight.x == 3);
        check("r.botRight.y == 4", r.botRight.y == 4);
    }

    // -------------------------------------------------------
    printf("\n--- 27. Struct array init ---\n");
    // -------------------------------------------------------
    {
        struct point arr[3] = {{10, 20}, {30, 40}, {50, 60}};
        check("arr[0].x == 10", arr[0].x == 10);
        check("arr[0].y == 20", arr[0].y == 20);
        check("arr[1].x == 30", arr[1].x == 30);
        check("arr[1].y == 40", arr[1].y == 40);
        check("arr[2].x == 50", arr[2].x == 50);
        check("arr[2].y == 60", arr[2].y == 60);
    }

    // -------------------------------------------------------
    printf("\n--- 28. Partial struct array init ---\n");
    // -------------------------------------------------------
    {
        struct point arr[3] = {{1, 2}};
        check("arr[0].x == 1", arr[0].x == 1);
        check("arr[0].y == 2", arr[0].y == 2);
        check("arr[1].x == 0", arr[1].x == 0);
        check("arr[1].y == 0", arr[1].y == 0);
        check("arr[2].x == 0", arr[2].x == 0);
        check("arr[2].y == 0", arr[2].y == 0);
    }

    // -------------------------------------------------------
    printf("\n--- 29. Mixed struct member types ---\n");
    // -------------------------------------------------------
    {
        struct mixed m = {65, 1000};
        check("m.c == 65", m.c == 65);
        check("m.i == 1000", m.i == 1000);
    }

    // -------------------------------------------------------
    // Summary
    // -------------------------------------------------------
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n", passed + failed);
    if (failed == 0)
        printf("ALL TESTS PASSED.\n");
    else
        printf("*** THERE WERE FAILURES ***\n");

    if (failed) getchar();
    return failed;
}
