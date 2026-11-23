struct Context {
    unsigned char reserved[0x2a];
    int attribute;
};

extern struct Context *Graf;

int far pascal hSetFont(int value)
{
    int old = Graf->attribute;
    Graf->attribute = value;
    return old;
}
