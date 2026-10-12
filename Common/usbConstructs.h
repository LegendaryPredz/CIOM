#ifndef _USB_CONSTRUCTS_H_
#define _USB_CONSTRUCTS_H_


//_____ D E F I N I T I O N S ______________________________________________

#define NB_MS_BEFORE_FLUSH		100
#define RXBUF_SIZE				MAX_PACKET_SIZE
#define TXBUF_SIZE				MAX_PACKET_SIZE

// Circular Buffer Sizes
#define RX_MAX_QUEUE			0x180    // 384 bytes
#define	TX_MAX_QUEUE			0x180    // 384 bytes

typedef struct PACKET_HEADER_t
{
	BYTE	Cmd;
	BYTE	SeqNum;
} PACKET_HEADER;


//_____ M A C R O S ________________________________________________________

// Queue Management Macros
#define _InitRxQueue		  RxHead = RxTail = (RX_MAX_QUEUE - 1)
#define _RxQueueFull		  (RxHead == ((RxTail == (RX_MAX_QUEUE - 1)) ? 0 : (RxTail + 1)))
#define _RxQueueEmpty		  (RxHead == RxTail)
#define _EnRxQueue(x)		  RxQueue[(RxTail = ((RxTail == (RX_MAX_QUEUE - 1)) ? 0 : (RxTail + 1)))] = x
#define _WaitRxQueueNotEmpty  while(_RxQueueEmpty)
///#define _WaitRxQueueNotEmpty  while(_RxQueueEmpty) {if(++BlinkRate == BLINK_LOOP){BlinkRate=0;HB_LED = ~HB_LED;}} 

#define RTN_STATUS(x)         EnTxQueue( (uint8_t)(x >> 8) & 0x00FF); EnTxQueue( (uint8_t)(x & 0x00FF));


#define _InitTxQueue          TxHead = TxTail = (TX_MAX_QUEUE - 1)
#define _TxQueueFull          (TxHead == ((TxTail == (TX_MAX_QUEUE - 1)) ? 0 : (TxTail + 1))) 
#define _TxQueueEmpty         (TxHead == TxTail)
#define _EnTxQueue(x)         TxQueue[(TxTail = ((TxTail == (TX_MAX_QUEUE - 1)) ? 0 : (TxTail + 1)))] = x
#define _WaitTxQueueNotEmpty  while(_TxQueueEmpty)


//_____ D E C L A R A T I O N S ____________________________________________

extern WORD  RxHead;
extern WORD  RxTail;
extern WORD  TxHead;
extern WORD  TxTail;
extern PACKET_HEADER Header;   


extern BYTE cdcSendDataWaitTilDone(BYTE* dataBuf, WORD size, BYTE intfNum, ULONG ulTimeout);
extern BYTE cdcSendDataInBackground(BYTE* dataBuf, WORD size, BYTE intfNum, ULONG ulTimeout);
extern WORD cdcReceiveDataInBuffer(BYTE*,WORD,BYTE);

// Found in usbEventHandling.c
extern void USB_Task( void);
extern void InitQueues( void);
extern void EnRxQueue( BYTE);
extern void EnTxQueue( BYTE);
extern void SendTxQueue( void);
extern BYTE GetNextRxByte( void);
extern BYTE	GetNextTxByte( void);


#endif  // _USB_CONSTRUCTS_H_
