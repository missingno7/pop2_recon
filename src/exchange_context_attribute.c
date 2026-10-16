struct Context {
    unsigned char reserved[0x2a];
    int attribute;
};

extern struct Context *CURRENT_CONTEXT;

int far pascal EXCHANGE_CONTEXT_ATTRIBUTE(int value)
{
    int old = CURRENT_CONTEXT->attribute;
    CURRENT_CONTEXT->attribute = value;
    return old;
}
