struct Pair { int x; int y; };
struct Rec { struct Pair a; int gap[21]; struct Pair b; };

/* Candidate for the two 16-bit field pairs at offsets 0 and 0x2e. */
void far f(struct Rec near *p)
{
    struct Pair temporary;
    temporary = p->a;
    p->a = p->b;
    p->b = temporary;
}
