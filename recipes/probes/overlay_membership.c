int far pascal is_selected_code(int value)
{
    int result;
    switch (value == 15 || value == 109 || value == 7 || value == 11 ||
            value == 13 || value == 38 || value == 44 || value == 28 ||
            value == 158 || value == 170 || value == 171) {
    case 1:
        result = 1;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}
