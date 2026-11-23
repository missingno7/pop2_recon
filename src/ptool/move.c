struct Context {
    unsigned char reserved[0x22];
    int x;
    int y;
};

extern struct Context *Graf;

void far pascal Move(int y_delta, int x_delta)
{
    struct Context *context = Graf;
    context->y += y_delta;
    context->x += x_delta;
}
