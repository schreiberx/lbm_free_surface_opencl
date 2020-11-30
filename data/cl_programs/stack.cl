__kernel void main(
		__global int stack_push[MAX_STACK_SIZE],
		__global int stack_delete[MAX_STACK_SIZE]
		)
{
	const size_t gid = get_global_id(0);

	int items_on_stack_delete = stack_delete[0];

	// load the index of the item on stack_push which was deleted
	int deleted_item_on_stack_push = stack_delete[gid];

	/*
	 * the deletable items are one less since the first element is
	 * the counter for the items on the stack
	 */
	int deletable_items = items_on_stack_delete-1;

	/*
	 * immediately return if this thread is not intended to fill any gaps
	 */
	if (gid > deletable_items)
		return;

	int items_on_stack_push = stack_push[0];

	/*
	 * there are at most deletable_items on the top of the stack.
	 * this also means, that there have to be at least deletable_items
	 * undeleted items within the "2*deletable_items" topmost items.
	 */
	int lower_limit = items_on_stack_push - deletable_items;

	int i = items_on_stack_push-1;
	int found_items = 0;
	for (; i >= lower_limit; i--)
	{
		/*
		 * increment 'found_items' for each valid item which state was
		 * not set to deleted
		 */
		if (stack_push[i] != -1)
			found_items++;

		/*
		 * if we found 'gid' valid items, we are happy and use this
		 * to fill the gap which index is stored at stack_delete[gid]
		 */
		if (found_items == gid)
			break;
	}

#if 1
	if (i < lower_limit)	{/*ERROR*/}
#endif

	stack_push[stack_delete[gid]] = stack_push[i];
}
