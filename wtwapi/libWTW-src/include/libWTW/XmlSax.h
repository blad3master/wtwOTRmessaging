/*
*** Copyright (C) 2007-2015, K2T.eu
*/

#pragma once
#include "Core.h"

namespace wtw
{

	class LIBWTW_API CXmlSax
	{
	protected:
		void * m_pParser;

	public:
		CXmlSax();
		virtual ~CXmlSax();

	public:
		HRESULT create(const wchar_t *pszEncoding = NULL, const wchar_t *pszSep = NULL);
		void destroy();

	public:
		HRESULT parse(const char *pszBuffer, int nLength = -1, int bIsFinal = 1);
		HRESULT parse(const wchar_t *pszBuffer, int nLength = -1, int bIsFinal = 1);
		HRESULT parseBuffer(int nLength, int bIsFinal = 1);
		void *getBuffer(int nLength);

	public:
		HRESULT enableStartElementHandler(int bEnable = 1);
		HRESULT enableEndElementHandler(int bEnable = 1);
		HRESULT enableElementHandler(int bEnable = 1);
		HRESULT enableCharacterDataHandler(int bEnable = 1);
		HRESULT enableProcessingInstructionHandler(int bEnable = 1);
		HRESULT enableCommentHandler(int bEnable = 1);
		HRESULT enableStartCdataSectionHandler(int bEnable = 1);
		HRESULT enableEndCdataSectionHandler(int bEnable = 1);
		HRESULT enableCdataSectionHandler(int bEnable = 1);
		HRESULT enableDefaultHandler(int bEnable = 1, int bExpand = 1);
		HRESULT enableExternalEntityRefHandler(int bEnable = 1);
		HRESULT enableUnknownEncodingHandler(int bEnable = 1);
		HRESULT enableStartNamespaceDeclHandler(int bEnable = 1);
		HRESULT enableEndNamespaceDeclHandler(int bEnable = 1);
		HRESULT enableNamespaceDeclHandler(int bEnable = 1);
		HRESULT enableXmlDeclHandler(int bEnable = 1);
		HRESULT enableStartDoctypeDeclHandler(int bEnable = 1);
		HRESULT enableEndDoctypeDeclHandler(int bEnable = 1);
		HRESULT enableDoctypeDeclHandler(int bEnable = 1);

	public:
		int getErrorCode();
		HRESULT getCurrentByteIndex(long &);
		HRESULT getCurrentLineNumber(int &);
		HRESULT getCurrentColumnNumber(int &);
		HRESULT getCurrentByteCount(int &);
		const wchar_t *getErrorString();

	public:
		static const wchar_t *getVersion();
		static void getVersion(int *pnMajor, int *pnMinor, int *pnMicro);
		static const wchar_t *getErrorString(int nError);

	protected:
		virtual void onStartElement(const wchar_t *pszName, const wchar_t **papszAttrs);
		virtual void onEndElement(const wchar_t *pszName);
		virtual void onCharacterData(const wchar_t *pszData, int nLength);
		virtual void onProcessingInstruction(const wchar_t *pszTarget, const wchar_t *pszData);
		virtual void onComment(const wchar_t *pszData);
		virtual void onStartCdataSection();
		virtual void onEndCdataSection();
		virtual void onDefault(const wchar_t *pszData, int nLength);
		virtual int onExternalEntityRef(const wchar_t *pszContext, const wchar_t *pszBase, const wchar_t *pszSystemID, const wchar_t *pszPublicID);
		//virtual int onUnknownEncoding(const wchar_t *pszName, XML_Encoding *pInfo);
		virtual void onStartNamespaceDecl(const wchar_t *pszPrefix, const wchar_t *pszURI);
		virtual void onEndNamespaceDecl(const wchar_t *pszPrefix);
		virtual void onXmlDecl(const wchar_t *pszVersion, const wchar_t *pszEncoding, int bStandalone);
		virtual void onStartDoctypeDecl(const wchar_t *pszDoctypeName, const wchar_t *pszSysID, const wchar_t *pszPubID, int bHasInternalSubset);
		virtual void onEndDoctypeDecl();

	protected:
		virtual void onPostCreate();

	private:
		static void __cdecl _startElementHandler(void *cbData, const wchar_t *pszName, const wchar_t **papszAttrs);
		static void __cdecl _endElementHandler(void *cbData,	const wchar_t *pszName);
		static void __cdecl _characterDataHandler(void *cbData, const wchar_t *pszData, int nLength);
		static void __cdecl _processingInstructionHandler(void *cbData, const wchar_t *pszTarget, const wchar_t *pszData);
		static void __cdecl _commentHandler(void *cbData,	const wchar_t *pszData);
		static void __cdecl _startCdataSectionHandler(void *cbData);
		static void __cdecl _endCdataSectionHandler(void *cbData);
		static void __cdecl _defaultHandler(void *cbData, const wchar_t *pszData, int nLength);
		static int __cdecl _externalEntityRefHandler(void *cbData, const wchar_t *pszContext, const wchar_t *pszBase, const wchar_t *pszSystemID, const wchar_t *pszPublicID);
		//static int __cdecl _unknownEncodingHandler(void *cbData, const wchar_t *pszName, XML_Encoding *pInfo);
		static void __cdecl _startNamespaceDeclHandler(void *cbData, const wchar_t *pszPrefix, const wchar_t *pszURI);
		static void __cdecl _endNamespaceDeclHandler(void *cbData, const wchar_t *pszPrefix);
		static void __cdecl _xmlDeclHandler(void *cbData, const wchar_t *pszVersion, const wchar_t *pszEncoding, int nStandalone);
		static void __cdecl _startDoctypeDeclHandler(void *cbData, const wchar_t *pszDoctypeName, const wchar_t *pszSysID, const wchar_t *pszPubID, int nHasInternalSubset);
		static void __cdecl _endDoctypeDeclHandler(void *cbData);
	};
}