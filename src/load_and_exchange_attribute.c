extern unsigned int far pascal EXCHANGE_CONTEXT_ATTRIBUTE(unsigned int attribute);
extern unsigned int far pascal LOAD_FONT_ATTRIBUTE(unsigned int attribute);

unsigned int far pascal LOAD_AND_EXCHANGE_ATTRIBUTE(unsigned int attribute)
{
    return EXCHANGE_CONTEXT_ATTRIBUTE(LOAD_FONT_ATTRIBUTE(attribute));
}
