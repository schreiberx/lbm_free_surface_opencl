

#if LONELEY_INTERFACE_REMOVER
	if (flag == FLAG_INTERFACE)
	{
		int neighbor_flags = 0;
	#define STD_STUFF2	neighbor_flags |= flag_array[dd_index];

		// we leave the fluid mass and fluid fraction unchanged cz. those should be already around 1.0

		// check neighbored cells if they are fluid cells and convert them to interface cells
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X);			STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X);			STD_STUFF2;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_Y);			STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_Y);			STD_STUFF2;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_Z);			STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_Z);			STD_STUFF2;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y);	STD_STUFF2;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z);	STD_STUFF2;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z);	STD_STUFF2;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z);	STD_STUFF2;

	#undef STD_STUFF2

#define FLAGS_FLUID_AND_GAS	(FLAG_FLUID | FLAG_GAS)
		// do nothing if cell is connected to a fluid AND a gas cell
		if ((neighbor_flags & FLAGS_FLUID_AND_GAS) == FLAGS_FLUID_AND_GAS)
			return;

		if (neighbor_flags & FLAG_FLUID)
		{
			// this interface is connected to gas cells only
			flag_array[gid] = FLAG_FLUID;
		}
	}
#endif
