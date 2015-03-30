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

	// return 4 OTRL policies + OTRL_POLICY_GLOBAL_SETTING
	static OtrlPolicy getPopupMenu(const wchar_t *id, const wchar_t *netClass, int netId);
	static OtrlPolicy getPopupMenu(const wtwContactDef *contact);

	// return 4 OTRL policies (if OTRL_POLICY_GLOBAL_SETTING is set, it is converted to one out of 4)
	static OtrlPolicy get(const wchar_t *id, const wchar_t *netClass, int netId);
	static OtrlPolicy get(const wtwContactDef *contact);

	static void set(const wtwContactDef *contatct, OtrlPolicy policy);

private:
	
	static wtwOTRmessaging *pwtwOTRmessaging;

	static std::map<std::wstring, OtrlPolicy> cache;

	static OtrlPolicy settingsGet(const wchar_t *contact_key);

	static void settingsSet(const wchar_t *contact_key, OtrlPolicy policy);

};

