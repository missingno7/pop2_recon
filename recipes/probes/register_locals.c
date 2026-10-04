int register_locals(int a, int b, int c, int d)
{
    register int left = a + b;
    register int right = c + d;
    return left * right + a;
}
