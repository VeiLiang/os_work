#ifndef __TRIGGER_ARRAY_STACK_H__
#define __TRIGGER_ARRAY_STACK_H__
#include "types.h"
#include "rxchip.h"

//#define STACK_MAX_COUNT         4
//#define STACK_INDEX_IDLE        -1
//#define STACK_INDEX_POP
//////////////触发器
#define TRIG_1  0x01
#define TRIG_2  0x02
#define TRIG_3  0x04
#define TRIG_4  0x08

#define HX_TRIG1        "trig1"
#define HX_TRIG2        "trig2"
#define HX_TRIG3        "trig3"
#define HX_TRIG4        "trig4"

#define TRIGGER_DELAY_BEGIN 1
#define TRIGGER_DELAY_END   0xff


#define PARKING_TRIG    TRIG_2
#define HX_PARKING_TRIG HX_TRIG2



//静态栈 单例
enum
{
    STACK_INDEX_0,
    STACK_INDEX_1,
    STACK_INDEX_2,
    STACK_INDEX_3,
    STACK_MAX_COUNT,
    STACK_INDEX_IDLE,
    STACK_INDEX_POP,
};

typedef struct trigger_data_node
{
    int index;  //栈中位置 0开始;
    u8  trig_index;
    int delay_time;
}TRIGGER_DATA_NODE, *TRIGGER_DATA_NODE_PTR;

////////////////////////
//
typedef struct trigger_stack
{
    TRIGGER_DATA_NODE stack[STACK_MAX_COUNT];
    int stack_size;
    int top_index;  //逻辑栈顶 在数组中的位置 0开始
}TRIGGER_STACK, *TRIGGER_STACK_PTR;

//栈顶出栈返回
TRIGGER_DATA_NODE_PTR pop_trigger_data();

//入栈
int push_trigger_data(u8 trig_index,int delay_time);

//删除
void remove_trigger_data_by_name(u8 trig_index);

//查找元素
TRIGGER_DATA_NODE_PTR fine_trigger_data_by_name(u8 trig_index);

//获取栈顶元素 不出栈
TRIGGER_DATA_NODE_PTR get_top_trigger_data();

//获取栈元素个数
int get_trigger_stack_size();

//初始化栈
void init_trigger_stack();

void printf_stack_info();

u8 log2(u32 val);
int trig_on(u8 trig_index);
void trig_off(u8 trig_index);
void   trig_detect_init();


#endif



