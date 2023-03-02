// SPDX-FileCopyrightText: 2021 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOCUTFILESWORKER_H
#define DOCUTFILESWORKER_H

#include "dfmplugin_fileoperations_global.h"
#include "fileoperations/fileoperationutils/abstractworker.h"
#include "fileoperations/fileoperationutils/fileoperatebaseworker.h"

#include "dfm-base/interfaces/abstractjobhandler.h"
#include "dfm-base/interfaces/abstractfileinfo.h"

#include "dfm-io/core/dfile.h"

#include <QObject>

DPFILEOPERATIONS_BEGIN_NAMESPACE
class StorageInfo;

class DoCutFilesWorker : public FileOperateBaseWorker
{
    friend class CutFiles;
    Q_OBJECT
    explicit DoCutFilesWorker(QObject *parent = nullptr);

public:
    virtual ~DoCutFilesWorker() override;

protected:
    bool doWork() override;
    void stop() override;
    bool initArgs() override;
    void onUpdateProgress() override;

    bool cutFiles();
    bool doCutFile(const AbstractFileInfoPointer &fromInfo, const AbstractFileInfoPointer &targetPathInfo);
    bool doRenameFile(const AbstractFileInfoPointer &sourceInfo, const AbstractFileInfoPointer &targetPathInfo, AbstractFileInfoPointer &toInfo, bool *ok);
    bool renameFileByHandler(const AbstractFileInfoPointer &sourceInfo, const AbstractFileInfoPointer &targetInfo);

    void emitCompleteFilesUpdatedNotify(const qint64 &writCount);

private:
    bool checkSymLink(const AbstractFileInfoPointer &fromInfo);
    bool checkSelf(const AbstractFileInfoPointer &fromInfo);
};
DPFILEOPERATIONS_END_NAMESPACE

#endif   // DOCUTFILESWORKER_H