"""Worker-local relocation-order model for ordinary 16-bit RTLink MZ links."""


def predict_order(module, segment, *, bucket_order=None):
    """Predict the order of segment-word relocations in one input contribution.

    With no ``bucket_order``, return {(target_kind, target_name): offsets}.
    Each tuple of offsets is ordered. Mapping iteration has no order meaning.
    With an independently known permutation of those keys, return the flat
    tuple of offsets. Offsets are relative to the named input SEGDEF.

    Use for the tested resident relocatable-frame modes only. The object does
    not establish whether an external ultimately resolves to an absolute or
    overlay target, or which output segment and global bucket rank it gets.
    Segment target identities in this result are local to this module.
    """
    if segment not in module.segment_lengths:
        raise ValueError(f'unknown input segment {segment!r}')
    groups = {}
    used = set()
    for record in module.fixups:
        for fix in record['resolved']:
            if fix['kind'] != 'fixup' or fix['segment'] != segment:
                continue
            loc = fix['loc_type']
            if loc not in (2, 3):
                continue
            target = fix['target']
            frame = fix['frame']
            if frame['kind'] == 'target_frame':
                frame = frame['target']
            key = target['kind'], target.get('name')
            if (fix['self_relative'] or key[0] not in ('external', 'group', 'segment')
                    or fix['displacement'] is not None
                    or (frame['kind'], frame.get('name')) != key):
                raise ValueError(f'unsupported MZ fixup equation at {segment}:{fix["segment_offset"]:#x}')
            if key[0] == 'segment' and module.segment_defs[target['index'] - 1]['frame'] is not None:
                raise ValueError(f'absolute segment target {key[1]!r}')
            site = fix['segment_offset'] + (2 if loc == 3 else 0)
            if site in used:
                raise ValueError(f'duplicate relocation site in {segment}:{site:#x}')
            used.add(site)
            base16, pointer1616 = groups.setdefault(key, ([], []))
            (base16 if loc == 2 else pointer1616).append(site)

    orders = {key: tuple(base16 + pointer1616) for key, (base16, pointer1616) in groups.items()}
    if bucket_order is None:
        return orders
    keys = tuple(bucket_order)
    if len(keys) != len(orders) or set(keys) != orders.keys():
        raise ValueError('bucket_order must list each predicted bucket exactly once')
    return tuple(site for key in keys for site in orders[key])
