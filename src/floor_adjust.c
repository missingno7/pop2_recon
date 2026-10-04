/* Semantic name hypothesis only; original symbol remains unrecovered. */
void far pascal floor_adjust(register int value, int near *value_out) {
    if (value < 0)
        --value;
    *value_out = value;
}
