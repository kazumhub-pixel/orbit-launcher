// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QCryptographicHash>
#include <QFile>
#include "tasks/Task.h"
class ElyAgentCheck : public Task {
    QString m_file;
protected:
    void executeTask() override {
        if(verify(m_file))emitSucceeded();
        else emitFailed("A biblioteca Ely.by está ausente ou foi alterada. Reinstale o pacote Orbit completo.");
    }
public:
    explicit ElyAgentCheck(QString file):m_file(std::move(file)){}
    static bool verify(const QString& path){QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>8*1024*1024)return false;return QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256).toHex()=="9c7f4343e6c82034958ffb48c14a2cb0c85928be7283103ce17da00c6d5a7b10";}
};
