int signed_branch(int left, int right)
{
    if (left < right)
        return -1;
    if (left == right)
        return 0;
    return 1;
}
