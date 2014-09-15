#ifndef __SM_H__
#define __SM_H__

#define CONFIG_SM_DEBUG 		1

#ifndef CONFIG_SM_FSM
#define CONFIG_SM_FSM 			1
#endif

#ifndef CONFIG_SM_HSM
#define CONFIG_SM_HSM 			1
#endif

#if ! (CONFIG_SM_FSM || CONFIG_SM_HSM)
#error "FSM and HSM must chose one!"
#endif

#ifndef SM_MAX_NEST_DEPTH
#define SM_MAX_NEST_DEPTH 		6
#endif

#ifndef offsetof
#define offsetof(type, member) ((unsigned int) &((type *)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member)  ((type *)( (char *)(ptr) - offsetof(type,member) ))
#endif

#define sm_entry()

typedef int sm_sig_t;

/**
 * @bref 状态机事件
 */
typedef struct sm_event_s
{
	sm_sig_t sig;
	void *event;
}sm_event_t;

/**
 * @bref 状态处理函数返回值, 指示事件被怎么处理了
 */
typedef unsigned char sm_ret_t;

//struct sm_fsm_s;
typedef struct sm_s sm_t;
typedef sm_ret_t (*sm_state_handler_t)(sm_t *fsm, sm_event_t const *e);

/**
 * @bref 状态机
 */
struct sm_s
{
	sm_state_handler_t state;
	sm_state_handler_t temp;
};

/**
 * @bref 状态机返回值
 *
 */
enum
{
	SM_RET_HANDLED,
	SM_RET_IGNORE,
	SM_RET_UNHANDLED,

	SM_RET_TRAN,
	SM_RET_SUPER,
};
#define SM_RET_CAST(x) 	( (sm_ret_t)(x) )

#define SM_HANDLED() 		SM_RET_CAST(SM_RET_HANDLED)
#define SM_IGNORE() 		SM_RET_CAST(SM_RET_IGNORE)
#define SM_UNHANDLED() 	SM_RET_CAST(SM_RET_UNHANDLED)

#define SM_TRAN(me, target) \
			((me)->temp = (target), SM_RET_CAST(SM_RET_TRAN))
#define SM_SUPER(me, super) \
			((me)->temp = (super), SM_RET_CAST(SM_RET_SUPER))


enum sm_reserved_sig
{
	SM_EMPTY_SIG = -5,
	SM_ENTRY_SIG = -4,
	SM_EXIT_SIG  = -3,
	SM_INIT_SIG  = -2,
	SM_USER_SIG  = -1,
};
extern sm_event_t sm_reserved_event[];
#define SM_TRIG(me, state, sig) 	((state)(me, &sm_reserved_event[5 + (sig)]))
#define SM_ENTRY(me, state) 		SM_TRIG(me, state, SM_ENTRY_SIG)
#define SM_EXIT(me, state) 		SM_TRIG(me, state, SM_EXIT_SIG)

#if CONFIG_SM_FSM
void fsm_ctor(sm_t *me, sm_state_handler_t init);
sm_ret_t  fsm_init(sm_t *me, sm_event_t *e);
void fsm_dispatch(sm_t *me, sm_event_t *e);
#endif

#if CONFIG_SM_HSM
void hsm_ctor(sm_t *me, sm_state_handler_t init);
sm_ret_t hsm_top(sm_t *me, const sm_event_t *e);
void hsm_init(sm_t *me, sm_event_t *e);
void hsm_dispatch(sm_t *me, sm_event_t *e);
#endif

#if CONFIG_SM_DEBUG
#define SM_ASSERT(cond) 	do{ if(!(cond)){ while(1); } }while(0)
#else
#define SM_ASSERT(cond) 	/* NULL */
#endif

#endif
