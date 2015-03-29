#pragma once

// Custom Encryption Layer

#include "wtwPlugin.h"
#include "wtwContact.h"

struct wtwCELMessage
{
	__wtwStdStructDefs(wtwCELMessage);

	int			structSize;

	wtwContactDef contactData;
	
	const wchar_t *pProtocolGuid;
	
	void *		pText;		// wtw::CString *

	DWORD		flags;
};

#define WTW_CEL_RET_OK			S_OK	// continue normal processing
#define WTW_CEL_RET_CONSUME		1		// eat/drop this message
#define WTW_CEL_RET_ENCRYPTED	2		// message was decrypted succesfully, replace text and contine processing
#define WTW_CEL_RET_DECRYPTED	3		// message was encrypted, replace with encrypted text and send it 

/* Hooks */

#define WTW_CEL_MESSAGE_RECEIVED		L"WTW/CEL/MsgReceived"	// wP = wtwCELMessage *
#define WTW_CEL_MESSAGE_BEFORE_SEND		L"WTW/CEL/MsgSend"		// wP = wtwCELMessage *