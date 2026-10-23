// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QByteArray>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif
namespace ElyTokenVault {
inline QByteArray protect(const QByteArray& input, bool decrypt=false) {
#ifdef Q_OS_WIN
    DATA_BLOB in{static_cast<DWORD>(input.size()),reinterpret_cast<BYTE*>(const_cast<char*>(input.constData()))}, out{};
    const bool ok=decrypt?CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out):CryptProtectData(&in,L"Orbit Ely.by OAuth",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out);
    if(!ok)return {};
    QByteArray result(reinterpret_cast<char*>(out.pbData),static_cast<qsizetype>(out.cbData));SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);return result;
#else
    Q_UNUSED(input);Q_UNUSED(decrypt);return {};
#endif
}
}
