#ifndef __TRIGGER_ARRAY_STACK_H__
#define __TRIGGER_ARRAY_STACK_H__
#include "types.h"
#include "rxchip.h"

//#define STACK_MAX_COUNT         4
//#define STACK_INDEX_IDLE        -1
//#define STACK_INDEX_POP
//////////////������
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



//��̬ջ ����
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
    int index;  //ջ��λ�� 0��ʼ;
    u8  trig_index;
    int delay_time;
}TRIGGER_DATA_NODE, *TRIGGER_DATA_NODE_PTR;

////////////////////////
//
typedef struct trigger_stack
{
    TRIGGER_DATA_NODE stack[STACK_MAX_COUNT];
    int stack_size;
    int top_index;  //�߼�ջ�� �������е�λ�� 0��ʼ
}TRIGGER_STACK, *TRIGGER_STACK_PTR;

//ջ����ջ����
TRIGGER_DATA_NODE_PTR pop_trigger_data();

//��ջ
int push_trigger_data(u8 trig_index,int delay_time);

//ɾ��
void remove_trigger_data_by_name(u8 trig_index);

//����Ԫ��
TRIGGER_DATA_NODE_PTR fine_trigger_data_by_name(u8 trig_index);

//��ȡջ��Ԫ�� ����ջ
TRIGGER_DATA_NODE_PTR get_top_trigger_data();

//��ȡջԪ�ظ���
int get_trigger_stack_size();

//��ʼ��ջ
void init_trigger_stack();

void printf_stack_info();

u8 log2(u32 val);
int trig_on(u8 trig_index);
void trig_off(u8 trig_index);
void   trig_detect_init();


#endif



