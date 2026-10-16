/* Forward the far pointer to the separately reviewed block-flag helper. */
extern void far pascal CLEAR_FAR_BLOCK_FLAGS(void far *block);

void far pascal CLEAR_FLAGS_OF_FAR_BLOCK(void far *block)
{
    CLEAR_FAR_BLOCK_FLAGS(block);
}
