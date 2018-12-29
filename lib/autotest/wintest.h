#ifndef _WINTEST_H_
#define _WINTEST_H_

#define	MAX_STACKSTAGE		6
#define	MAX_NAME				15				/* Maximum name length.*/
#define	MAX_KEYS				200			/* Maximum number of keyboad keys.*/
#define	MAX_CHAR				200

#define	SCRIPT_TEST			0				// �﷨���״̬
#define	SCRIPT_MAKE			1				// ������������״̬

#define	COMP_MASK			0x80000000
#define	KEYDOWN_MASK		0x40000000
#define	SEQBEGIN_MASK		0x20000000
#define	CHAR_MASK			0x10000000	// �ַ���Ϣ

#define	UNICODE_MASK		0x0000FFFF
#define	KEYVALUE_MASK		0x000000FF


typedef struct _TESTER_COMMAND {
	char *		CommandCode;		 			// String of Command
	int			(*DoIt)(void);					// Function to process command.
} TESTER_COMMAND;

typedef struct tagLOOPINFO
{
	long		LoopCnt;
	i64_t		LoopPos;
} LOOPINFO;

typedef struct tagKEYS
{
	UCHAR		KeyName[MAX_NAME];				// �������
	UCHAR		KeyValue;							// �����ֵ
	UCHAR		KeyLength;							// Length of the name
	UCHAR		LogName[MAX_NAME];				// �߼�����
	UCHAR		LogLength;							// Length of the name
} KEYS;

int TesterGetMessage		(XMTESTERMESSAGE *pTesterMessage);
int TesterOpenScript (const char * lpScriptFile, const char * lpKeyMapFile);
int TesterCloseScript (void);
const char * TesterGetKeyName (DWORD key);
int TesterKeymap (char * pKeyMapFileName);


#endif