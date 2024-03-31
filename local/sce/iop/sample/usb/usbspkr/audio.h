/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*			Copyright (C) 2001 Sony Computer Entertainment
 *			All Right Reserved
 *
 * audio.h - USB audio class definitions
 *
 * $Id: audio.h,v 1.1 2001/12/01 10:35:34 fukunaga Exp $
 */

/* Audio Interface Subclass Codes */
#define USB_SUBCLASS_UNDEFINED		0x00
#define USB_SUBCLASS_AUDIOCONTROL	0x01
#define USB_SUBCLASS_AUDIOSTREAMING	0x02
#define USB_SUBCLASS_MIDISTREAMING	0x03

/* Audio Class-Specific Descriptor Types */
#define USB_CS_UNDEFINED		0x20
#define USB_CS_DEVICE			0x21
#define USB_CS_CONFIGURATION		0x22
#define USB_CS_STRING			0x23
#define USB_CS_INTERFACE		0x24
#define USB_CS_ENDPOINT			0x25

/* Audio Class-Specific AC Interface Descriptor Subtypes */
#define USB_AS_DESCRIPTOR_UNDEFINED	0x00
#define USB_AS_GENERAL			0x01
#define USB_FORMAT_TYPE			0x02
#define USB_FORMAT_SPECIFIC		0x03

/* Format Type Codes */
#define USB_FORMAT_TYPE_UNDEFINED	0x00
#define USB_FORMAT_TYPE_I		0x01
#define USB_FORMAT_TYPE_II		0x02
#define USB_FORMAT_TYPE_III		0x03

typedef struct {	/* Type I Format Type Descriptor */
	u_char bDescLength;
	u_char bDescriptorType;		/* CS_INTERFACE */
	u_char bDescriptorSubType;	/* FORMAT_TYPE */
	u_char bFormatType;		/* FORMAT_TYPE_I */
	u_char bNrChannels;
	u_char bSubframeSize;
	u_char bBitResolution;
	u_char bSamFreqType;
	struct {
		u_char tSamFreq0;
		u_char tSamFreq1;
		u_char tSamFreq2;
	} SamplingFrequencyTables[0];
} UsbTypeIFormatTypeDescriptor;

