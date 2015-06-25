#pragma once

#define QUERY_INTERFACE_CAST(x)\
	if (riid == __uuidof(x)) {\
		x* pObj = static_cast<x*>(this);\
		pObj->AddRef();\
		*ppvObject = pObj;\
		return S_OK;\
	}

#define QUERY_INTERFACE_FAIL() *ppvObject = nullptr; return E_NOINTERFACE;