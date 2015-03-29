/*
** Copyright (C) 2007-2015, K2T.eu
*/

#pragma once

#include "..\..\libWTWCommon.h"
#include "String.h"
#include "BaseObject.h"
#include <deque>

namespace wtw
{
	namespace sqlite
	{
		class LIBWTW_API CConnection : public CBaseObject
		{
			friend class CCommand;

		protected:
			static int onBusyCallback(void *, int);

			struct sqlite3 *m_db;
			int    m_bInTransaction;
			int    m_busyRetry;
			int    m_busyDelay;

		public:
			HRESULT getLastError(DWORD & code);
			HRESULT getLastError(const wchar_t **ret);

		public:
			HRESULT transactionBegin();
			HRESULT transactionCommit();
			HRESULT transactionRollback();
			HRESULT inTrasaction();

		public:
			CConnection();
			CConnection(const wchar_t * dbPath);
			virtual ~CConnection();
			HRESULT open(const wchar_t *dbPath);
			HRESULT close();
			HRESULT columnExists(const wchar_t *table, const wchar_t *column);
			HRESULT getLastInsertId(unsigned long long &id);
			HRESULT getLastInsertId(long long &id);
			HRESULT getLastInsertId(unsigned int &id);
			HRESULT getLastInsertId(int &id);
			bool	isOpened();
			HRESULT	releaseMemory();

		public:
			HRESULT exec(const wchar_t*sql);
			HRESULT query(const wchar_t *sql, int & ret);
			HRESULT query(const wchar_t *sql, long long & ret);
			HRESULT query(const wchar_t *sql, double & ret);
#if defined(__HAVE_WTW_STRING__)
			HRESULT query(const wchar_t *sql, wtw::CString & ret);
#endif
			HRESULT query(const wchar_t *sql, const wchar_t **);

			HRESULT getAffectedRowNum(int &rows, int bTotal = 0);

			HRESULT enableWAL(int bEnable);
			HRESULT enableBusyHandler(int maxRetry, int delay);
		};

		class LIBWTW_API CCommand : public CBaseObject
		{
			friend class CReader;

		protected:
			CConnection &	m_con;
			struct sqlite3_stmt *m_statament;
			int				m_argc;
			int				m_refs;

		public:
			CCommand(CConnection & con, const wchar_t*sql);
			CCommand(CConnection & con);
			virtual ~CCommand();

			HRESULT prepare(const wchar_t *sql);
			HRESULT finalize();

			HRESULT bind(int index);	// 1 - based
			HRESULT bind(int index, const int & data);	// 1 - based
			HRESULT bind(int index, const long long & data);	// 1 - based
			HRESULT bind(int index, const double & data);	// 1 - based
			HRESULT bind(int index, const wchar_t *data, int dataLen = -1);	// 1 - based
			HRESULT bind(int index, const char *data, int dataLen = -1);	// 1 - based
			HRESULT bind(int index, const void *data, int dataLen);	// 1 - based

			HRESULT query(int & ret);	// get single result from first row
			HRESULT query(long long & ret);
			HRESULT query(double & ret);
#if defined(__HAVE_WTW_STRING__)
			HRESULT query(wtw::CString & ret);
#endif

			HRESULT exec(); // execute without result
			CReader query(int bQuery = 1);
		};

		class LIBWTW_API CReader : public CBaseObject
		{
			friend class CCommand;

		protected:

			CCommand *m_cmd;
			CReader(CCommand *cmd, int bCountRows = 1);

		protected:	// this should be private, never use
			HRESULT count(int & count);
			int m_intptr;
			int m_rowCount;

		public:
			CReader();
			CReader(const CReader &copy);
			virtual ~CReader();

			CReader& operator = (const CReader &copy);

			HRESULT read(); // returns S_OK when successful, E_* on error, S_FALSE - no more data
			HRESULT reset();
			HRESULT close();

			HRESULT getCurrentRowIndex(int & i);
			HRESULT getRowNum(int & i);

			HRESULT hasNext(); // returns S_OK when successful, E_* on error, S_FALSE - no more data

			HRESULT get(int index, int &result, int defaultVal = 0);	// 0 based
			HRESULT get(int index, long long &result, long long defaultVal = 0LL);	// 0 based
			HRESULT get(int index, double &result, double defaultVal = 0.f);	// 0 based
#if defined(__HAVE_WTW_STRING__)
			HRESULT get(int index, wtw::CString &result, const wtw::CString & defaultVal = L"");	// 0 based
#endif
			HRESULT get(int index, const wchar_t** result, const wchar_t *defaultVal = L"");	// 0 based
			HRESULT get(int index, const char** result, const char *defaultVal = "");	// 0 based
			HRESULT get(int index, void **data, int & dataLen);	// 0 based

			HRESULT get(const wchar_t *colName, void **data, int & dataLen);
			HRESULT get(const wchar_t *colName, int &result, int defaultVal = 0);
			HRESULT get(const wchar_t *colName, long long &result, long long defaultVal = 0LL);
			HRESULT get(const wchar_t *colName, double &result, double defaultVal = 0.f);
#if defined(__HAVE_WTW_STRING__)
			HRESULT get(const wchar_t *colName, wtw::CString &result, const wtw::CString & defaultVal = L"");
#endif
			HRESULT get(const wchar_t *colName, const wchar_t** result, const wchar_t *defaultVal = L"");
			HRESULT get(const wchar_t *colName, const char** result, const char *defaultVal = "");

			HRESULT getLength(int index, int & size);
			HRESULT getLength(const wchar_t *colName, int & size);

			HRESULT getColumnCount(int &count);
#if defined(__HAVE_WTW_STRING__)
			HRESULT getColumnName(int index, wtw::CString & name);
#endif
			HRESULT getColumnName(int index, const wchar_t **name);

		private:
			std::deque<void*> m_toFree;

			void releaseIntPointers();
		};
	};
};