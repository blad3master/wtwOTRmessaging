/*
*** Copyright (C) 2007-2014, K2T.eu
*/

#pragma once

#include "..\wtwPlugin.h"
#include "Core.h"
#include "BaseObject.h"

namespace wtw
{
	class LIBWTW_API CSyncObject abstract : public CBaseObject
	{
	public:
		virtual HRESULT lock(int dwWaitTime) = 0;
		virtual HRESULT unlock() = 0;
	};

	class LIBWTW_API CEvent : public CSyncObject
	{
//#if __cplusplus > 199711L 
	public:
		CEvent(const CEvent&) = delete;
//#endif

	public:
		CEvent();
		CEvent(const wchar_t *name);
		virtual ~CEvent();

		virtual HRESULT lock(int dwWaitTime) override;
		virtual HRESULT unlock() override;

		HRESULT setEvent();
		HRESULT resetEvent();

	public:
		void *m_hObject;
	};

	class LIBWTW_API CCriticalSection : public CSyncObject
	{
//#if __cplusplus > 199711L 
	public:
		CCriticalSection(const CCriticalSection&) = delete;
//#endif

	protected:
		void *m_pcs;

	public:
		CCriticalSection();
		virtual ~CCriticalSection();

		virtual HRESULT lock(int dwWaitTime = INFINITE) override;
		virtual HRESULT unlock() override;
	};

	class LIBWTW_API CSingleLock
	{
//#if __cplusplus > 199711L 
	public:
		CSingleLock(const CSingleLock&) = delete;
//#endif
	protected:
		CSyncObject *m_pObject;
		int m_lockCount;

	public:
		CSingleLock(CSyncObject *pObj);
		virtual ~CSingleLock();

		virtual HRESULT lock(int dwWaitTime = INFINITE);
		virtual HRESULT unlock();
	};
};