#include "utils.h"
#include "PeerPolicy.h"
#include "PluginSettings.h"

wtwOTRmessaging *PeerPolicy::pwtwOTRmessaging = nullptr;

std::map<std::wstring, OtrlPolicy> PeerPolicy::cache;


PeerPolicy::PeerPolicy()
{
}


PeerPolicy::~PeerPolicy()
{
}

void PeerPolicy::init(wtwOTRmessaging *mainClassPtr)
{
	pwtwOTRmessaging = mainClassPtr;
}


OtrlPolicy PeerPolicy::get(const wchar_t *id, const wchar_t *netClass, int netId)
{
	OtrlPolicy policy;
	const auto &key = makeKeyFromPeer(id, netClass, netId);
	const auto it = cache.find(key);
	if (it != cache.end())
	{
		policy = it->second;
	}
	else
	{
		policy = settingsGet(key.c_str());
		cache[key] = policy;
	}

	if (OTRL_POLICY_GLOBAL_SETTING == policy)
	{
		SettingsBroker::OTRL_POLICY p = pwtwOTRmessaging->instance->getSettingsBroker().getOtrlPolicy();
		switch (p) {
			case SettingsBroker::OTRL_POLICY::NEVER:			policy = OTRL_POLICY_NEVER;			break;
			case SettingsBroker::OTRL_POLICY::MANUAL:			policy = OTRL_POLICY_MANUAL;		break;
			case SettingsBroker::OTRL_POLICY::OPPORTUNISTIC:	policy = OTRL_POLICY_OPPORTUNISTIC;	break;
			case SettingsBroker::OTRL_POLICY::ALWAYS:			policy = OTRL_POLICY_ALWAYS;		break;
			default:
				policy = OTRL_POLICY_NEVER;
				LOG_CRITICAL(L"%s() unknow policy '%d'", __FUNCTIONW__, (int)p);
		}
	}

	{
		const wchar_t *p_text = L"unknown";
		switch (policy) {
		case OTRL_POLICY_NEVER:			p_text = L"NEVER";			break;
		case OTRL_POLICY_MANUAL:		p_text = L"MANUAL";			break;
		case OTRL_POLICY_OPPORTUNISTIC:	p_text = L"OPPORTUNISTIC";	break;
		case OTRL_POLICY_ALWAYS:		p_text = L"ALWAYS";			break;
		}
		LOG_TRACE(L"%s() policy for user '%s' => '%s'", __FUNCTIONW__, key.c_str(), p_text);
	}

	return policy;
}

OtrlPolicy PeerPolicy::get(const wtwContactDef *contact)
{
	return get(contact->id, contact->netClass, contact->netId);
}

void PeerPolicy::set(const wtwContactDef *contact, OtrlPolicy policy)
{
	const auto &key = makeKeyFromPeer(contact);
	auto it = cache.find(key);
	if ((it == cache.end()) || (policy != it->second))
	{
		settingsSet(key.c_str(), policy);
		cache[key] = policy;
	}
}

OtrlPolicy PeerPolicy::settingsGet(const wchar_t *contact_key)
{
	return PluginSettings::getInt(contact_key, OTRL_POLICY_GLOBAL_SETTING);
}

void PeerPolicy::settingsSet(const wchar_t *contact_key, OtrlPolicy policy)
{
	if (false == PluginSettings::setInt(contact_key, policy))
	{
		LOG_ERROR(L"%s() failed to save policy for user '%s'", __FUNCTIONW__, contact_key);
	}
}
