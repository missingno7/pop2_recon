extern int helper(int);

int invoke_external(int value)
{
    return helper(value) + 1;
}
