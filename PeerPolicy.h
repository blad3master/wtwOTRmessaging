#pragma once

#include <string>
#include <map>
#include "OTRL\proto.h"
#include "wtwapi\wtwContact.h"
#include "wtwOTRmessaging.h"

const OtrlPolicy OTRL_POLICY_GLOBAL_SETTING = 0x1ffffff1;	// some number which is different from OTRL policies

class PeerPolicy
{
public:
	PeerPolicy();
	~PeerPolicy();

	static void init(wtwOTRmessaging *mainClassPtr);

	static OtrlPolicy get(const wchar_t *id, const wchar_t *netClass, int netId);
	static OtrlPolicy get(const wtwContactDef *contact);

	static void set(const wtwContactDef *contatct, OtrlPolicy policy);

private:
	
	static wtwOTRmessaging *pwtwOTRmessaging;

	static std::map<std::wstring, OtrlPolicy> cache;

	static OtrlPolicy settingsGet(const wchar_t *contact_key);

	static void settingsSet(const wchar_t *contact_key, OtrlPolicy policy);

};

