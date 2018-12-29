#include "trigger_array_stack.h"
#include "xm_printf.h"
#include "types.h"



static struct trigger_stack g_trigger_stack;


//2^n=x,输入X,返回N
//如果有几个触发,得到的是最高位的触发
u8 log2(u32 val)
{
    int res = 31;
    
    if(val == 0)
    {
        res = 0xff;
    }
    else
    {
        for(;res > 0;res -- )
        {
            if((val >> res ) == 1)
            {
                break;
            }
        }
    }
    return res;
}

void printf_stack_info()
{
    int i = 0;
    for(i = 0; i<STACK_MAX_COUNT ;i ++)
    {
        printf("-----------------> index :%d \n",g_trigger_stack.stack[i].index);
        printf("-----------------> trig_index : %d \n",g_trigger_stack.stack[i].trig_index);
        printf("-----------------> delay_time : %d \n",g_trigger_stack.stack[i].delay_time);
    }
    printf("---------------------------> stack_size : %d \n",g_trigger_stack.stack_size);
    printf("---------------------------> top_index : %d \n",g_trigger_stack.top_index);
}

static int get_top_index(const struct trigger_stack *stack)
{
    int index = STACK_INDEX_IDLE;
    if(stack->stack_size > 0)
    {
        int i = 0;
        for(i = 0; i < STACK_MAX_COUNT; i++)
        {
            if(stack->stack[i].index == stack->stack_size - 1)
            {
                index = i;
            }
        }
    }
    return index;
}

static int get_enable_index(struct trigger_stack *stack)
{
    int index = stack->top_index + 1;
    if(index >= STACK_MAX_COUNT)
    {
        int i = 0;
        for(i = 0;i < STACK_MAX_COUNT; i++)
        {
            if(stack->stack[i].index == STACK_INDEX_IDLE)
            {
                index = i;
                break;
            }
        }
    }
    return index;
}

static TRIGGER_DATA_NODE_PTR _pop(struct trigger_stack *stack)
{
    TRIGGER_DATA_NODE_PTR trigger_node_ptr = NULL;
    int top_index = stack->top_index;
    if(top_index != STACK_INDEX_IDLE)
    {
        trigger_node_ptr = &(stack->stack[top_index]);
        stack->stack[top_index].index = STACK_INDEX_IDLE;

    }
    stack->stack_size -= 1 ;
    stack->top_index = get_top_index(stack);
    //printf_stack_info();
    return trigger_node_ptr;
}

static int _push_by_trig_data(struct trigger_stack *stack,u8 trig_index,int delay_time)
{
    int index = get_enable_index(stack);
    if(index < STACK_MAX_COUNT )
    {
        stack->stack[index].delay_time = delay_time;
        stack->stack[index].trig_index = trig_index;
        stack->stack[index].index = stack->stack_size;
        stack->top_index = index;
        stack->stack_size += 1;
    }
    printf_stack_info();
    return index;
}

static void _remove_by_name(struct trigger_stack *stack,u8 trig_index)
{
    int i = 0;
    for(i = 0; i < STACK_MAX_COUNT ;i ++)
    {
        if(stack->stack[i].index != STACK_INDEX_IDLE)
        {
            if(stack->stack[i].trig_index == trig_index)
            {
                stack->stack[i].trig_index = 0xff;
                stack->stack[i].delay_time = 0;
                stack->stack_size -= 1;
                if(stack->top_index != i)   //非栈顶出栈要重新标记栈元素位置
                {
                    int j = 0;
                    for(j = 0; j<STACK_MAX_COUNT;j++)
                    {
                        if(stack->stack[j].index < STACK_MAX_COUNT
                            && stack->stack[j].index > stack->stack[i].index)
                        {
                            stack->stack[j].index -= 1;
                        }
                    }
                }
                stack->stack[i].index = STACK_INDEX_IDLE;
                stack->top_index = get_top_index(stack);
            }
        }
    }
    //printf_stack_info();
}

static TRIGGER_DATA_NODE_PTR _find_data_by_name(const struct trigger_stack *stack,u8 trig_index)
{
    int i = 0;
    TRIGGER_DATA_NODE_PTR trigger_node_ptr = NULL;
    for(i = 0; i < STACK_MAX_COUNT ;i ++)
    {
        if(stack->stack[i].index != STACK_INDEX_IDLE)
        {
            if( stack->stack[i].trig_index == trig_index )
            {
                trigger_node_ptr = (TRIGGER_DATA_NODE_PTR)&(stack->stack[i]);
            }
        }
    }
    return trigger_node_ptr;
}

static TRIGGER_DATA_NODE_PTR _get_top_data(const struct trigger_stack *stack)
{
    TRIGGER_DATA_NODE_PTR trigger_node_ptr = NULL;
    int top_index = stack->top_index;
    if(top_index != STACK_INDEX_IDLE)
    {
        trigger_node_ptr = (TRIGGER_DATA_NODE_PTR)&(stack->stack[top_index]);
    }
    return trigger_node_ptr;
}


//把索引都初始化为STACK_INDEX_IDLE
static void _init_trigger_stack(struct trigger_stack *stack)
{
    memset(stack,0,sizeof(*stack));
    int i = 0;
    for(i = 0; i < STACK_MAX_COUNT; i++)
    {
        stack->stack[i].index = STACK_INDEX_IDLE;
        stack->stack[i].trig_index = 0xff;
    }
    stack->top_index = STACK_INDEX_IDLE;
}

TRIGGER_DATA_NODE_PTR pop_trigger_data()
{
    return _pop(&g_trigger_stack);
}

int push_trigger_data(u8 trig_index,int delay_time)
{
    return _push_by_trig_data(&g_trigger_stack,trig_index,delay_time);
}

void remove_trigger_data_by_name(u8 trig_index)
{
    _remove_by_name(&g_trigger_stack,trig_index);
}

TRIGGER_DATA_NODE_PTR fine_trigger_data_by_name(u8 trig_index)
{
    return _find_data_by_name(&g_trigger_stack,trig_index);
}

TRIGGER_DATA_NODE_PTR get_top_trigger_data()
{
    return _get_top_data(&g_trigger_stack);
}

int get_trigger_stack_size()
{
    return g_trigger_stack.stack_size;
}

void init_trigger_stack()
{
    _init_trigger_stack(&g_trigger_stack);
    //printf_stack_info();
}



