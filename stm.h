#ifndef __STM_H__
#define __STM_H__

#define CONFIG_STM_DEBUG 		1

#ifndef CONFIG_STM_FSM
#define CONFIG_STM_FSM 			1
#endif

#ifndef CONFIG_STM_HSM
#define CONFIG_STM_HSM 			1
#endif

#if ! (CONFIG_STM_FSM || CONFIG_STM_HSM)
#error "FSM and HSM must chose one!"
#endif

#ifndef STM_MAX_NEST_DEPTH
#define STM_MAX_NEST_DEPTH 		6
#endif

#ifndef offsetof
#define offsetof(type, member) ((unsigned int) &((type *)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member)  ((type *)( (char *)(ptr) - offsetof(type,member) ))
#endif

#define stm_entry()

typedef int stm_sig_t;

/**
 * @bref 状态机事件
 */
typedef struct stm_event_s
{
	stm_sig_t sig;
	void *event;
}stm_event_t;

/**
 * @bref 状态处理函数返回值, 指示事件被怎么处理了
 */
typedef unsigned char stm_ret_t;

//struct stm_fsm_s;
typedef struct stm_s stm_t;
typedef stm_ret_t (*stm_state_handler_t)(stm_t *fsm, stm_event_t const *e);

/**
 * @bref 状态机
 */
struct stm_s
{
	stm_state_handler_t state;
	stm_state_handler_t temp;
};

/**
 * @bref 状态机返回值
 *
 */
enum
{
	STM_RET_HANDLED,
	STM_RET_IGNORE,
	STM_RET_UNHANDLED,

	STM_RET_TRAN,
	STM_RET_SUPER,
};
#define STM_RET_CAST(x) 	( (stm_ret_t)(x) )

#define STM_HANDLED() 		STM_RET_CAST(STM_RET_HANDLED)
#define STM_IGNORE() 		STM_RET_CAST(STM_RET_IGNORE)
#define STM_UNHANDLED() 	STM_RET_CAST(STM_RET_UNHANDLED)

#define STM_TRAN(me, target) \
			((me)->temp = (target), STM_RET_CAST(STM_RET_TRAN))
#define STM_SUPER(me, super) \
			((me)->temp = (super), STM_RET_CAST(STM_RET_SUPER))


enum stm_reserved_sig
{
	STM_EMPTY_SIG = -5,
	STM_ENTRY_SIG = -4,
	STM_EXIT_SIG  = -3,
	STM_INIT_SIG  = -2,
	STM_USER_SIG  = -1,
};
extern stm_event_t stm_reserved_event[];
#define STM_TRIG(me, state, sig) 	((state)(me, &stm_reserved_event[5 + (sig)]))
#define STM_ENTRY(me, state) 		STM_TRIG(me, state, STM_ENTRY_SIG)
#define STM_EXIT(me, state) 		STM_TRIG(me, state, STM_EXIT_SIG)

#if CONFIG_STM_FSM
void fsm_ctor(stm_t *me, stm_state_handler_t init);
stm_ret_t  fsm_init(stm_t *me, stm_event_t *e);
void fsm_dispatch(stm_t *me, stm_event_t *e);
#endif

#if CONFIG_STM_HSM
void hsm_ctor(stm_t *me, stm_state_handler_t init);
stm_ret_t hsm_top(stm_t *me, const stm_event_t *e);
void hsm_init(stm_t *me, stm_event_t *e);
void hsm_dispatch(stm_t *me, stm_event_t *e);
#endif

#if CONFIG_STM_DEBUG
#define STM_ASSERT(cond) 	do{ if(!(cond)){ while(1); } }while(0)
#else
#define STM_ASSERT(cond) 	/* NULL */
#endif

#endif
