#include 	"stm.h"

stm_event_t stm_reserved_event[] =
{
	{ STM_EMPTY_SIG, 0 },
	{ STM_ENTRY_SIG, 0 },
	{ STM_EXIT_SIG,  0 },
	{ STM_INIT_SIG,  0 },
	{ STM_USER_SIG,  0 },
};


#if CONFIG_STM_FSM
void fsm_ctor(stm_t *me, stm_state_handler_t init)
{
	STM_ASSERT(0 != me);
	STM_ASSERT(0 != init);

	me->state = 0;
	me->temp  = init;
}
stm_ret_t  fsm_init(stm_t *me, stm_event_t *e)
{
	stm_ret_t ret;

	STM_ASSERT(0 != me);
	STM_ASSERT(0 != me->temp);

	ret = (me->temp)(me, e);
	if(ret != STM_RET_TRAN)
	{
		return ret;
	}

	STM_ENTRY(me, me->temp);

	me->state = me->temp;

	return ret;
}
void fsm_dispatch(stm_t *me, stm_event_t *e)
{
	stm_ret_t ret;

	STM_ASSERT(me->state == me->temp);

	ret = (me->temp)(me, e);
	if(STM_RET_TRAN == ret)
	{
		STM_EXIT(me, me->state);
		STM_ENTRY(me, me->temp);
		me->state = me->temp;
	}
}
#endif

#if CONFIG_STM_HSM
/**
 * @brief HSM椤剁姸鎬�
 */
stm_ret_t hsm_top(stm_t *me, const stm_event_t *e)
{
	(void)me;
	(void)e;

	return STM_IGNORE();
}

static unsigned char hsm_find_path(stm_t *me,
						stm_state_handler_t t,
						stm_state_handler_t s,
						stm_state_handler_t path[STM_MAX_NEST_DEPTH])
{
	signed char ip = -1;
	signed char iq;
	stm_ret_t ret;

	/* (a) check source==target (transition to self) */
	if( s == t)
	{
		STM_EXIT(me, s);
		ip = 0;
	}
	else
	{
		STM_TRIG(me, t, STM_EMPTY_SIG);
		t = me->temp;

		/* (b) check source==target->super */
		if( s == t )
		{
			ip = 0;
		}
		else
		{
			STM_TRIG(me, s, STM_EMPTY_SIG);

			/* (c) check source->super==target->super */
			if(me->temp == t)
			{
				STM_EXIT(me, s);
				ip = 0;
			}
			else
			{
				/* (d) check source->super==target */
				if( me->temp == path[0] )
				{
					STM_EXIT(me, s);
				}
				else
				{
					/* (e) check rest of source==target->super->super..
					 * and store the entry path along the way
					 */
					ip = 1;
					iq = 0;
					path[1] = t;
					t = me->temp;

					/* find target->super->super... */
					ret = STM_TRIG(me, path[1], STM_EMPTY_SIG);
					while(STM_RET_SUPER == ret)
					{
						path[++ip] = me->temp;
						if(s == me->temp)
						{
							iq = 1;
							STM_ASSERT(ip < STM_MAX_NEST_DEPTH);
							ip--;

							ret = STM_RET_HANDLED;
						}
						else
						{
							ret = STM_TRIG(me, me->temp, STM_EMPTY_SIG);
						}
					}

					/* the LCA not found yet? */
					if(0 == iq)
					{
						STM_ASSERT(ip < STM_MAX_NEST_DEPTH);

						STM_EXIT(me, s);

						/* (f) check the rest of source->super
						 *                  == target->super->super...
						 */
						iq = ip;
						ret = STM_RET_IGNORE; /* LCA NOT found */
						do
						{
							s = path[iq];
							/* is this the LCA? */
							if(t == s)
							{
								ret = STM_RET_HANDLED;

								ip = iq - 1;
								iq = -1;
							}
							else
							{
								iq--; /* try lower superstate of target */
							}
						}while(iq >= 0);

						 /* LCA not found? */
						if( STM_RET_HANDLED != ret )
						{
							/* (g) check each source->super->...
							 * for each target->super...
							 */
							ret = STM_RET_IGNORE;
							do
							{
								if(STM_RET_HANDLED  == STM_EXIT(me, t))
								{
									STM_TRIG(me, t, STM_EMPTY_SIG);
								}
								t = me->temp;
								iq = ip;
								do
								{
									s = path[iq];
									if( t == s)
									{
										ip = iq -1;
										iq = -1;

										ret = STM_RET_HANDLED; /* break */
									}
									else
									{
										iq--;
									}
								}while(iq >= 0);
							}while(STM_RET_HANDLED != ret);
						}
					}
				}
			}
		}
	}

	return ip;
}

void hsm_ctor(stm_t *me, stm_state_handler_t init)
{
	STM_ASSERT(0 != me);
	STM_ASSERT(0 != init);

	me->state = hsm_top;
	me->temp  = init;
}
void hsm_init(stm_t *me, stm_event_t *e)
{
	stm_ret_t ret;
	signed char ip;

	stm_state_handler_t path[STM_MAX_NEST_DEPTH];
	stm_state_handler_t t = me->state;

	STM_ASSERT(0 != me);
	STM_ASSERT(0 != me->temp);
	STM_ASSERT(hsm_top == t);

	ret = (me->temp)(me, e);
	STM_ASSERT(STM_RET_TRAN == ret);

	do
	{
		ip = 0;

		path[0] = me->temp;
		STM_TRIG(me, me->temp,STM_EMPTY_SIG);
		while( t != me->temp )
		{
			path[++ip] = me->temp;
			STM_TRIG(me, me->temp,STM_EMPTY_SIG);
		}
		me->temp = path[0];

		STM_ASSERT(ip < STM_MAX_NEST_DEPTH);

		do
		{
			STM_ENTRY(me, path[ip--]);
		}while(ip >= 0);

		t = path[0];
	}while(STM_RET_TRAN == STM_TRIG(me, t, STM_INIT_SIG));

	me->temp = t;
	me->state = me->temp;
}


void hsm_dispatch(stm_t *me, stm_event_t *e)
{
	stm_state_handler_t t = me->state;
	stm_state_handler_t s;

	stm_ret_t ret;

	// 鐘舵�佸繀椤荤ǔ瀹�
	STM_ASSERT(me->state == me->temp);

	/* process the event hierarchically... */
	// 浜嬩欢閫掑綊瑙﹀彂, 鐩村埌鏌愪釜鐘舵�佸鐞嗚浜嬩欢
	do
	{
		s = me->temp;
		ret = s(me, e); 	// 璋冪敤鐘舵�佸鐞嗗嚱鏁�
		if(STM_RET_UNHANDLED == ret)
		{
			ret = STM_TRIG(me, s, STM_EMPTY_SIG);
		}
	}while(STM_RET_SUPER == ret);

	// 濡傛灉鍙戠敓鐘舵�佽浆鎹�
	if(STM_RET_TRAN == ret)
	{
		stm_state_handler_t path[STM_MAX_NEST_DEPTH];
		signed char ip = -1;

		path[0] = me->temp; 	// 鐘舵�佽浆鎹㈢殑鐩殑鐘舵��
		path[1] = t; 			// 鐘舵�佽浆鎹㈢殑婧愮姸鎬�

		/* exit current state to transition source s... */
		for( ; s != t; t = me->temp)
		{
			ret = STM_EXIT(me, t);
			if(STM_RET_HANDLED == ret)
			{
				STM_TRIG(me, t, STM_EMPTY_SIG);
			}
		}

		ip = hsm_find_path(me, path[0], s, path);

		for(; ip>=0; ip--)
		{
			STM_ENTRY(me, path[ip]);
		}

		t = path[0];
		me->temp = t;

		/* drill into the target hierarchy... */
		while( STM_RET_TRAN == STM_TRIG(me, t, STM_INIT_SIG) )
		{
			ip = 0;
			path[0] = me->temp;

			STM_TRIG(me, me->temp, STM_EMPTY_SIG);
			while(t != me->temp)
			{
				path[++ip] = me->temp;
				STM_TRIG(me, me->temp, STM_EMPTY_SIG);
			}
			me->temp = path[0];

			STM_ASSERT(ip < STM_MAX_NEST_DEPTH);

			do
			{
				STM_ENTRY(me, path[ip--]);
			}while(ip >= 0);

			t = path[0];
		}// end: while( STM_RET_TRAN == STM_TRIG(me, t, STM_INIT_SIG) )
	} // end: if(STM_RET_TRAN == ret)

	me->temp = t;
	me->state = t;
}
#endif
