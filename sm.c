#include 	"sm.h"

sm_event_t sm_reserved_event[] =
{
	{ SM_EMPTY_SIG, 0 },
	{ SM_ENTRY_SIG, 0 },
	{ SM_EXIT_SIG,  0 },
	{ SM_INIT_SIG,  0 },
	{ SM_USER_SIG,  0 },
};


#if CONFIG_SM_FSM
void fsm_ctor(sm_t *me, sm_state_handler_t init)
{
	SM_ASSERT(0 != me);
	SM_ASSERT(0 != init);

	me->state = 0;
	me->temp  = init;
}
sm_ret_t  fsm_init(sm_t *me, sm_event_t *e)
{
	sm_ret_t ret;

	SM_ASSERT(0 != me);
	SM_ASSERT(0 != me->temp);

	ret = (me->temp)(me, e);
	if(ret != SM_RET_TRAN)
	{
		return ret;
	}

	SM_ENTRY(me, me->temp);

	me->state = me->temp;

	return ret;
}
void fsm_dispatch(sm_t *me, sm_event_t *e)
{
	sm_ret_t ret;

	SM_ASSERT(me->state == me->temp);

	ret = (me->temp)(me, e);
	if(SM_RET_TRAN == ret)
	{
		SM_EXIT(me, me->state);
		SM_ENTRY(me, me->temp);
		me->state = me->temp;
	}
}
#endif

#if CONFIG_SM_HSM
/**
 * @brief HSM根状态
 */
sm_ret_t hsm_top(sm_t *me, const sm_event_t *e)
{
	(void)me;
	(void)e;

	return SM_IGNORE();
}

static unsigned char hsm_find_path(sm_t *me,
						sm_state_handler_t t,
						sm_state_handler_t s,
						sm_state_handler_t path[SM_MAX_NEST_DEPTH])
{
	signed char ip = -1;
	signed char iq;
	sm_ret_t ret;

	/* (a) check source==target (transition to self) */
	if( s == t)
	{
		SM_EXIT(me, s);
		ip = 0;

		goto hsm_find_path_end;
	}

	SM_TRIG(me, t, SM_EMPTY_SIG);
	t = me->temp;

	/* (b) check source==target->super */
	if( s == t )
	{
		ip = 0;
		goto hsm_find_path_end;
	}

	SM_TRIG(me, s, SM_EMPTY_SIG);

	/* (c) check source->super==target->super */
	if(me->temp == t)
	{
		SM_EXIT(me, s);
		ip = 0;
		goto hsm_find_path_end;
	}

	/* (d) check source->super==target */
	if( me->temp == path[0] )
	{
		SM_EXIT(me, s);
		goto hsm_find_path_end;
	}

	/* (e) check rest of source==target->super->super..
	 * and store the entry path along the way
	 */
	ip = 1;
	iq = 0;
	path[1] = t;
	t = me->temp;

	/* find target->super->super... */
	ret = SM_TRIG(me, path[1], SM_EMPTY_SIG);
	while(SM_RET_SUPER == ret)
	{
		path[++ip] = me->temp;
		if(s == me->temp)
		{
			iq = 1;
			SM_ASSERT(ip < SM_MAX_NEST_DEPTH);
			ip--;

			ret = SM_RET_HANDLED;
		}
		else
		{
			ret = SM_TRIG(me, me->temp, SM_EMPTY_SIG);
		}
	}

	/* the LCA not found yet? */
	if(0 == iq)
	{
		SM_ASSERT(ip < SM_MAX_NEST_DEPTH);

		SM_EXIT(me, s);

		/* (f) check the rest of source->super
		 *                  == target->super->super...
		 */
		iq = ip;
		ret = SM_RET_IGNORE; /* LCA NOT found */
		do
		{
			s = path[iq];
			/* is this the LCA? */
			if(t == s)
			{
				ret = SM_RET_HANDLED;

				ip = iq - 1;
				iq = -1;
			}
			else
			{
				iq--; /* try lower superstate of target */
			}
		}while(iq >= 0);

		 /* LCA not found? */
		if( SM_RET_HANDLED != ret )
		{
			/* (g) check each source->super->...
			 * for each target->super...
			 */
			ret = SM_RET_IGNORE;
			do
			{
				if(SM_RET_HANDLED  == SM_EXIT(me, t))
				{
					SM_TRIG(me, t, SM_EMPTY_SIG);
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

						ret = SM_RET_HANDLED; /* break */
					}
					else
					{
						iq--;
					}
				}while(iq >= 0);
			}while(SM_RET_HANDLED != ret);
		}
	}

hsm_find_path_end:
	return ip;
}

void hsm_ctor(sm_t *me, sm_state_handler_t init)
{
	SM_ASSERT(0 != me);
	SM_ASSERT(0 != init);

	me->state = hsm_top;
	me->temp  = init;
}
void hsm_init(sm_t *me, sm_event_t *e)
{
	sm_ret_t ret;
	signed char ip;

	sm_state_handler_t path[SM_MAX_NEST_DEPTH];
	sm_state_handler_t t = me->state;

	SM_ASSERT(0 != me);
	SM_ASSERT(0 != me->temp);
	SM_ASSERT(hsm_top == t);

	ret = (me->temp)(me, e);
	SM_ASSERT(SM_RET_TRAN == ret);

	do
	{
		ip = 0;

		path[0] = me->temp;
		SM_TRIG(me, me->temp,SM_EMPTY_SIG);
		while( t != me->temp )
		{
			path[++ip] = me->temp;
			SM_TRIG(me, me->temp,SM_EMPTY_SIG);
		}
		me->temp = path[0];

		SM_ASSERT(ip < SM_MAX_NEST_DEPTH);

		do
		{
			SM_ENTRY(me, path[ip--]);
		}while(ip >= 0);

		t = path[0];
	}while(SM_RET_TRAN == SM_TRIG(me, t, SM_INIT_SIG));

	me->temp = t;
	me->state = me->temp;
}


void hsm_dispatch(sm_t *me, sm_event_t *e)
{
	sm_state_handler_t t = me->state;
	sm_state_handler_t s;

	sm_ret_t ret;

	// 状态必须稳定
	SM_ASSERT(me->state == me->temp);

	/* process the event hierarchically... */
	// 事件递归触发, 直到某个状态处理该事件
	do
	{
		s = me->temp;
		ret = s(me, e); 	// 调用状态处理函数
		if(SM_RET_UNHANDLED == ret)
		{
			ret = SM_TRIG(me, s, SM_EMPTY_SIG);
		}
	}while(SM_RET_SUPER == ret);

	// 如果发生状态转换
	if(SM_RET_TRAN == ret)
	{
		sm_state_handler_t path[SM_MAX_NEST_DEPTH];
		signed char ip = -1;

		path[0] = me->temp; 	// 状态转换的目的状态
		path[1] = t; 			// 状态转换的源状态

		/* exit current state to transition source s... */
		for( ; s != t; t = me->temp)
		{
			ret = SM_EXIT(me, t);
			if(SM_RET_HANDLED == ret)
			{
				SM_TRIG(me, t, SM_EMPTY_SIG);
			}
		}

		ip = hsm_find_path(me, path[0], s, path);

		for(; ip>=0; ip--)
		{
			SM_ENTRY(me, path[ip]);
		}

		t = path[0];
		me->temp = t;

		/* drill into the target hierarchy... */
		while( SM_RET_TRAN == SM_TRIG(me, t, SM_INIT_SIG) )
		{
			ip = 0;
			path[0] = me->temp;

			SM_TRIG(me, me->temp, SM_EMPTY_SIG);
			while(t != me->temp)
			{
				path[++ip] = me->temp;
				SM_TRIG(me, me->temp, SM_EMPTY_SIG);
			}
			me->temp = path[0];

			SM_ASSERT(ip < SM_MAX_NEST_DEPTH);

			do
			{
				SM_ENTRY(me, path[ip--]);
			}while(ip >= 0);

			t = path[0];
		}// end: while( SM_RET_TRAN == SM_TRIG(me, t, SM_INIT_SIG) )
	} // end: if(SM_RET_TRAN == ret)

	me->temp = t;
	me->state = t;
}
#endif
