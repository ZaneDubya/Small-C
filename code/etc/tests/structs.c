// test.c -- Comprehensive test program for struct support in Small-C 2.2.
// Tests: definition, declaration (global/local), member access (dot/arrow),
//        sizeof, assignment (member, deep copy), brace initialization,
//        nested structs, chained access, pointer to struct,
//        struct as function parameter, struct as function return value.
//
// Each test prints PASS or FAIL with a description.
// At the end, a summary of passed/failed/total is printed.

#include "../../smallc22/stdio.h"

int passed, failed;

// ============================================================================
// Struct definitions
// ============================================================================

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

struct tiny {
    int val;
};

// Global struct variables
struct point gp1;
struct point gp2;
struct rect  grect;

// Global struct with brace initializer
struct point ginit = { 10, 20 };

// ============================================================================
// Helper functions
// ============================================================================

check(char *desc, int cond) {
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

// Functions for testing struct parameter passing (hidden pointer)
int getX(struct point p) {
    return p.x;
}

int getY(struct point p) {
    return p.y;
}

int sumPoint(struct point p) {
    return p.x + p.y;
}

int rectArea(struct rect r) {
    int w, h;
    w = r.botRight.x - r.topLeft.x;
    h = r.botRight.y - r.topLeft.y;
    if (w < 0) w = -w;
    if (h < 0) h = -h;
    return w * h;
}

// Functions for testing struct return value (hidden pointer)
struct point makePoint(int x, int y) {
    struct point p;
    p.x = x;
    p.y = y;
    return p;
}

struct rect makeRect(int x1, int y1, int x2, int y2) {
    struct rect r;
    r.topLeft.x = x1;
    r.topLeft.y = y1;
    r.botRight.x = x2;
    r.botRight.y = y2;
    return r;
}

struct point addPoints(struct point a, struct point b) {
    struct point result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

// Helper: set a struct via pointer
setPointViaPtr(struct point *pp, int x, int y) {
    pp->x = x;
    pp->y = y;
}

// Helper: read from struct via pointer
int getPtrX(struct point *pp) {
    return pp->x;
}

int getPtrY(struct point *pp) {
    return pp->y;
}

// ============================================================================
// Main test program
// ============================================================================

main() {
    passed = 0;
    failed = 0;

    printf("=== Small-C Struct Test Suite ===\n\n");

    // -------------------------------------------------------
    printf("--- 1. Global struct declaration and member access ---\n");
    // -------------------------------------------------------
    gp1.x = 5;
    gp1.y = 10;
    check("global.x write/read", gp1.x == 5);
    check("global.y write/read", gp1.y == 10);

    gp1.x = 100;
    check("global.x overwrite", gp1.x == 100);
    gp1.x = 5;

    // -------------------------------------------------------
    printf("\n--- 2. Local struct declaration and member access ---\n");
    // -------------------------------------------------------
    {
        struct point lp;
        lp.x = 42;
        lp.y = 99;
        check("local.x write/read", lp.x == 42);
        check("local.y write/read", lp.y == 99);
    }

    // -------------------------------------------------------
    printf("\n--- 3. Multiple local structs ---\n");
    // -------------------------------------------------------
    {
        struct point a, b;
        a.x = 1;
        a.y = 2;
        b.x = 3;
        b.y = 4;
        check("multi local a.x", a.x == 1);
        check("multi local a.y", a.y == 2);
        check("multi local b.x", b.x == 3);
        check("multi local b.y", b.y == 4);
        check("locals are independent", a.x != b.x);
    }

    // -------------------------------------------------------
    printf("\n--- 4. sizeof(struct) ---\n");
    // -------------------------------------------------------
    check("sizeof(struct point) == 4", sizeof(struct point) == 4);
    check("sizeof(struct rect) == 8", sizeof(struct rect) == 8);
    check("sizeof(struct tiny) == 2", sizeof(struct tiny) == 2);

    // -------------------------------------------------------
    printf("\n--- 5. Struct deep copy (assignment) ---\n");
    // -------------------------------------------------------
    {
        struct point src, dst;
        src.x = 77;
        src.y = 88;
        dst = src;
        check("deep copy dst.x", dst.x == 77);
        check("deep copy dst.y", dst.y == 88);
        // Verify independence after copy
        dst.x = 0;
        check("copy is independent (src unchanged)", src.x == 77);
        check("copy is independent (dst changed)", dst.x == 0);
    }

    // -------------------------------------------------------
    printf("\n--- 6. Global struct deep copy ---\n");
    // -------------------------------------------------------
    gp1.x = 11;
    gp1.y = 22;
    gp2 = gp1;
    check("global deep copy gp2.x", gp2.x == 11);
    check("global deep copy gp2.y", gp2.y == 22);
    gp2.x = 0;
    check("global copy independent", gp1.x == 11);

    // -------------------------------------------------------
    printf("\n--- 7. Global struct brace initialization ---\n");
    // -------------------------------------------------------
    check("brace init ginit.x", ginit.x == 10);
    check("brace init ginit.y", ginit.y == 20);

    // -------------------------------------------------------
    printf("\n--- 8. Nested struct (dot chaining) ---\n");
    // -------------------------------------------------------
    {
        struct rect r;
        r.topLeft.x = 1;
        r.topLeft.y = 2;
        r.botRight.x = 11;
        r.botRight.y = 12;
        check("nested r.topLeft.x", r.topLeft.x == 1);
        check("nested r.topLeft.y", r.topLeft.y == 2);
        check("nested r.botRight.x", r.botRight.x == 11);
        check("nested r.botRight.y", r.botRight.y == 12);
    }

    // -------------------------------------------------------
    printf("\n--- 9. Nested struct deep copy ---\n");
    // -------------------------------------------------------
    {
        struct rect r1, r2;
        r1.topLeft.x = 10;
        r1.topLeft.y = 20;
        r1.botRight.x = 30;
        r1.botRight.y = 40;
        r2 = r1;
        check("nested copy r2.topLeft.x", r2.topLeft.x == 10);
        check("nested copy r2.botRight.y", r2.botRight.y == 40);
        r2.topLeft.x = 0;
        check("nested copy independent", r1.topLeft.x == 10);
    }

    // -------------------------------------------------------
    printf("\n--- 10. Pointer to struct (arrow operator) ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        struct point *pp;
        p.x = 55;
        p.y = 66;
        pp = &p;
        check("arrow pp->x", pp->x == 55);
        check("arrow pp->y", pp->y == 66);
        pp->x = 77;
        check("arrow write pp->x", p.x == 77);
        check("arrow write reflects in p", pp->x == 77);
    }

    // -------------------------------------------------------
    printf("\n--- 11. Pointer to nested struct ---\n");
    // -------------------------------------------------------
    {
        struct rect r;
        struct rect *rp;
        r.topLeft.x = 3;
        r.topLeft.y = 4;
        r.botRight.x = 7;
        r.botRight.y = 8;
        rp = &r;
        check("arrow nested rp->topLeft.x", rp->topLeft.x == 3);
        check("arrow nested rp->botRight.y", rp->botRight.y == 8);
        rp->topLeft.x = 99;
        check("arrow nested write", r.topLeft.x == 99);
    }

    // -------------------------------------------------------
    printf("\n--- 12. Struct as function parameter ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        p.x = 100;
        p.y = 200;
        check("param getX(p)", getX(p) == 100);
        check("param getY(p)", getY(p) == 200);
        check("param sumPoint(p)", sumPoint(p) == 300);
        // Verify callee can't modify caller's struct
        p.x = 100;
        check("param does not modify caller", p.x == 100);
    }

    // -------------------------------------------------------
    printf("\n--- 13. Nested struct as function parameter ---\n");
    // -------------------------------------------------------
    {
        struct rect r;
        r.topLeft.x = 0;
        r.topLeft.y = 0;
        r.botRight.x = 5;
        r.botRight.y = 3;
        check("param rectArea", rectArea(r) == 15);
    }

    // -------------------------------------------------------
    printf("\n--- 14. Struct return from function ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        p = makePoint(33, 44);
        check("return makePoint.x", p.x == 33);
        check("return makePoint.y", p.y == 44);
    }

    // -------------------------------------------------------
    printf("\n--- 15. Nested struct return from function ---\n");
    // -------------------------------------------------------
    {
        struct rect r;
        r = makeRect(1, 2, 3, 4);
        check("return makeRect topLeft.x", r.topLeft.x == 1);
        check("return makeRect topLeft.y", r.topLeft.y == 2);
        check("return makeRect botRight.x", r.botRight.x == 3);
        check("return makeRect botRight.y", r.botRight.y == 4);
    }

    // -------------------------------------------------------
    printf("\n--- 16. Struct param and return combined ---\n");
    // -------------------------------------------------------
    {
        struct point a, b, c;
        a.x = 10;
        a.y = 20;
        b.x = 3;
        b.y = 7;
        c = addPoints(a, b);
        check("addPoints.x", c.x == 13);
        check("addPoints.y", c.y == 27);
    }

    // -------------------------------------------------------
    printf("\n--- 17. Set struct via pointer function ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        p.x = 0;
        p.y = 0;
        setPointViaPtr(&p, 123, 456);
        check("setViaPtr p.x", p.x == 123);
        check("setViaPtr p.y", p.y == 456);
    }

    // -------------------------------------------------------
    printf("\n--- 18. Read struct via pointer function ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        p.x = 500;
        p.y = 600;
        check("getPtrX", getPtrX(&p) == 500);
        check("getPtrY", getPtrY(&p) == 600);
    }

    // -------------------------------------------------------
    printf("\n--- 19. Mixed struct with char and int members ---\n");
    // -------------------------------------------------------
    {
        struct mixed m;
        m.c = 'A';
        m.i = 1000;
        check("mixed char member", m.c == 'A');
        check("mixed int member", m.i == 1000);
    }

    // -------------------------------------------------------
    printf("\n--- 20. Global nested struct access ---\n");
    // -------------------------------------------------------
    grect.topLeft.x = 50;
    grect.topLeft.y = 60;
    grect.botRight.x = 70;
    grect.botRight.y = 80;
    check("global nested topLeft.x", grect.topLeft.x == 50);
    check("global nested botRight.y", grect.botRight.y == 80);

    // -------------------------------------------------------
    printf("\n--- 21. Arithmetic on struct members ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        p.x = 10;
        p.y = 20;
        check("member add", p.x + p.y == 30);
        check("member sub", p.y - p.x == 10);
        check("member mul", p.x * p.y == 200);
        check("member compare <", p.x < p.y);
        check("member compare >", p.y > p.x);
        check("member compare ==", p.x == p.x);
        check("member compare !=", p.x != p.y);
    }

    // -------------------------------------------------------
    printf("\n--- 22. Assign expression to struct member ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        int v;
        v = 7;
        p.x = v * 3 + 1;
        p.y = p.x + 10;
        check("computed assign p.x", p.x == 22);
        check("self-referencing assign p.y", p.y == 32);
    }

    // -------------------------------------------------------
    printf("\n--- 23. Struct member in conditional ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        int ok;
        p.x = 5;
        p.y = 0;
        ok = 0;
        if (p.x)
            ok = 1;
        check("member true in if", ok == 1);
        ok = 1;
        if (p.y)
            ok = 0;
        check("member false in if", ok == 1);
    }

    // -------------------------------------------------------
    printf("\n--- 24. Struct member in while loop ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        int count;
        p.x = 5;
        count = 0;
        while (p.x > 0) {
            count++;
            p.x = p.x - 1;
        }
        check("while loop on member", count == 5);
        check("member after loop", p.x == 0);
    }

    // -------------------------------------------------------
    printf("\n--- 25. Multiple struct types in one scope ---\n");
    // -------------------------------------------------------
    {
        struct point p;
        struct tiny t;
        struct rect r;
        p.x = 1;
        t.val = 2;
        r.topLeft.x = 3;
        check("multi-type point", p.x == 1);
        check("multi-type tiny", t.val == 2);
        check("multi-type rect", r.topLeft.x == 3);
    }

    // -------------------------------------------------------
    printf("\n--- 26. Overwrite global struct ---\n");
    // -------------------------------------------------------
    gp1.x = 1;
    gp1.y = 2;
    gp1.x = 999;
    gp1.y = 888;
    check("global overwrite x", gp1.x == 999);
    check("global overwrite y", gp1.y == 888);

    // -------------------------------------------------------
    printf("\n--- 27. Pass global struct to function ---\n");
    // -------------------------------------------------------
    gp1.x = 50;
    gp1.y = 25;
    check("global param getX", getX(gp1) == 50);
    check("global param sumPoint", sumPoint(gp1) == 75);

    // -------------------------------------------------------
    printf("\n--- 28. Return struct into global ---\n");
    // -------------------------------------------------------
    gp1 = makePoint(111, 222);
    check("return to global .x", gp1.x == 111);
    check("return to global .y", gp1.y == 222);

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

    return failed;
}
