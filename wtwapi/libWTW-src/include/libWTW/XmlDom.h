/*
*** Copyright (C) 2007-2015, K2T.eu
*/

#pragma once
#include "Core.h"
#include "XmlSax.h"
#include "String.h"
#include <deque>
#include <vector>

namespace wtw
{
	class LIBWTW_API CXmlAttribute : public CBaseObject
	{
	public:
		wtw::CString ns;
		wtw::CString name;
		wtw::CString value;
	};

	class LIBWTW_API CXmlObject : public CBaseObject
	{
	protected:
		std::deque<CXmlAttribute> m_attributes;
		std::vector<CXmlObject> m_childs;

		wtw::CString m_tagNs;
		wtw::CString m_tagName;
		wtw::CString m_tagData;

	public:
		CXmlObject();
		virtual ~CXmlObject();

	public:
		HRESULT attrFind(const wchar_t * attrName, const wchar_t * attrNs, wtw::CString & value);
		HRESULT attrSet(const wchar_t * attrName, const wchar_t * attrNs, const wtw::CString & value);
		HRESULT attrRemove(const wchar_t * attrName, const wchar_t * attrNs);

	public:
		HRESULT childGet(const wchar_t * tagName, const wchar_t * xmlns, DWORD flags, CXmlObject **ppRetObj)
		{
			return E_NOTIMPL;
		}

		HRESULT childAppend(const wchar_t *tagName, DWORD flags, CXmlObject **ppRetObj)
		{
			return E_NOTIMPL;
		}

		HRESULT childRemove(const wchar_t *tagName, DWORD flags)
		{
			return E_NOTIMPL;
		}
	};

	class LIBWTW_API CXmlDomParser : protected CXmlSax
	{
	};

};