/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lixiang<lixianga@uniontech.com>
 *
 * Maintainer: lixiang<lixianga@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "propertydialogutil.h"
#include "views/multifilepropertydialog.h"
#include "propertydialogmanager.h"

#include <QApplication>
#include <QScreen>
#include <QTimer>

DWIDGET_USE_NAMESPACE
DFMBASE_USE_NAMESPACE
using namespace dfmplugin_propertydialog;
const int kMaxPropertyDialogNumber = 16;
PropertyDialogUtil::PropertyDialogUtil(QObject *parent)
    : QObject(parent)
{
    closeIndicatorTimer = new QTimer(this);
    closeIndicatorTimer->setInterval(1000);
    closeAllDialog = new CloseAllDialog;
    closeAllDialog->setWindowIcon(QIcon::fromTheme("dde-file-manager"));
    connect(closeAllDialog, &CloseAllDialog::allClosed, this, &PropertyDialogUtil::closeAllFilePropertyDialog);
    connect(closeIndicatorTimer, &QTimer::timeout, this, &PropertyDialogUtil::updateCloseIndicator);
}

PropertyDialogUtil::~PropertyDialogUtil()
{
    filePropertyDialogs.clear();

    if (closeAllDialog) {
        closeAllDialog->deleteLater();
    }
}

void PropertyDialogUtil::showPropertyDialog(const QList<QUrl> &urls)
{
    for (const QUrl &url : urls) {
        QWidget *widget = createCustomizeView(url);
        if (widget) {
            widget->show();
            widget->activateWindow();
            QRect qr = qApp->primaryScreen()->geometry();
            QPoint pt = qr.center();
            pt.setX(pt.x() - widget->width() / 2);
            pt.setY(pt.y() - widget->height() / 2);
            widget->move(pt);
        } else {
            showFilePropertyDialog(urls);
            break;
        }
    }
}

void PropertyDialogUtil::showFilePropertyDialog(const QList<QUrl> &urls)
{
    int count = urls.count();
    if (count < kMaxPropertyDialogNumber) {
        for (const QUrl &url : urls) {
            int index = urls.indexOf(url);
            if (!filePropertyDialogs.contains(url)) {
                FilePropertyDialog *dialog = new FilePropertyDialog();
                dialog->selectFileUrl(url);
                dialog->filterControlView();
                filePropertyDialogs.insert(url, dialog);
                createControlView(url);
                connect(dialog, &FilePropertyDialog::closed, this, &PropertyDialogUtil::closeFilePropertyDialog);
                if (1 == count) {
                    QPoint pos = getPropertyPos(dialog->size().width(), dialog->height());
                    dialog->move(pos);
                } else {
                    QPoint pos = getPerportyPos(dialog->size().width(), dialog->size().height(), count, index);
                    dialog->move(pos);
                }
                dialog->show();
            } else {
                filePropertyDialogs.value(url)->show();
                filePropertyDialogs.value(url)->activateWindow();
            }
            filePropertyDialogs.value(url)->show();
        }

        if (urls.count() >= 2) {
            closeAllDialog->show();
            closeIndicatorTimer->start();
        }
    } else {
        MultiFilePropertyDialog *multiFilePropertyDialog = new MultiFilePropertyDialog(urls);
        multiFilePropertyDialog->show();
        multiFilePropertyDialog->moveToCenter();
        multiFilePropertyDialog->raise();
    }
}

/*!
 * \brief           Normal view control extension
 * \param index     Subscript to be inserted
 * \param widget    The view to be inserted
 */
void PropertyDialogUtil::insertExtendedControlFileProperty(const QUrl &url, int index, QWidget *widget)
{
    if (widget) {
        FilePropertyDialog *dialog = nullptr;
        if (filePropertyDialogs.contains(url)) {
            dialog = filePropertyDialogs.value(url);
        } else {
            dialog = new FilePropertyDialog();
        }
        dialog->insertExtendedControl(index, widget);
    }
}

/*!
 * \brief           Normal view control extension
 * \param widget    The view to be inserted
 */
void PropertyDialogUtil::addExtendedControlFileProperty(const QUrl &url, QWidget *widget)
{
    if (widget) {
        FilePropertyDialog *dialog = nullptr;
        if (filePropertyDialogs.contains(url)) {
            dialog = filePropertyDialogs.value(url);
        } else {
            dialog = new FilePropertyDialog();
        }
        dialog->addExtendedControl(widget);
    }
}

void PropertyDialogUtil::closeFilePropertyDialog(const QUrl url)
{
    if (filePropertyDialogs.contains(url)) {
        filePropertyDialogs.remove(url);
    }

    if (filePropertyDialogs.isEmpty())
        closeAllDialog->close();
}

void PropertyDialogUtil::closeAllFilePropertyDialog()
{
    QList<FilePropertyDialog *> dialogs = filePropertyDialogs.values();
    for (FilePropertyDialog *dialog : dialogs) {
        dialog->close();
    }
    closeIndicatorTimer->stop();
    closeAllDialog->close();
}

void PropertyDialogUtil::createControlView(const QUrl &url)
{
    QMap<int, QWidget *> controlView = createView(url);
    int count = controlView.keys().count();
    for (int i = 0; i < count; ++i) {
        QWidget *view = controlView.value(controlView.keys()[i]);
        if (controlView.keys()[i] == -1) {
            addExtendedControlFileProperty(url, view);
        } else {
            insertExtendedControlFileProperty(url, controlView.keys()[i], view);
        }
    }
}

void PropertyDialogUtil::updateCloseIndicator()
{
    qint64 size { 0 };
    int fileCount { 0 };

    for (FilePropertyDialog *d : filePropertyDialogs.values()) {
        size += d->getFileSize();
        fileCount += d->getFileCount();
    }

    closeAllDialog->setTotalMessage(size, fileCount);
}

PropertyDialogUtil *PropertyDialogUtil::instance()
{
    static PropertyDialogUtil propertyManager;
    return &propertyManager;
}

QMap<int, QWidget *> PropertyDialogUtil::createView(const QUrl &url)
{
    return PropertyDialogManager::instance().createExtensionView(url);
}

QWidget *PropertyDialogUtil::createCustomizeView(const QUrl &url)
{
    return PropertyDialogManager::instance().createCustomView(url);
}

QPoint PropertyDialogUtil::getPropertyPos(int dialogWidth, int dialogHeight)
{
    const QScreen *cursor_screen = Q_NULLPTR;
    const QPoint &cursor_pos = QCursor::pos();

    auto screens = qApp->screens();
    auto iter = std::find_if(screens.begin(), screens.end(), [cursor_pos](const QScreen *screen) {
        return screen->geometry().contains(cursor_pos);
    });

    if (iter != screens.end()) {
        cursor_screen = *iter;
    }

    if (!cursor_screen) {
        cursor_screen = qApp->primaryScreen();
    }

    int desktopWidth = cursor_screen->size().width();
    int desktopHeight = cursor_screen->size().height();
    int x = (desktopWidth - dialogWidth) / 2;

    int y = (desktopHeight - dialogHeight) / 2;

    return QPoint(x, y) + cursor_screen->geometry().topLeft();
}

QPoint PropertyDialogUtil::getPerportyPos(int dialogWidth, int dialogHeight, int count, int index)
{
    Q_UNUSED(dialogHeight)
    const QScreen *cursor_screen = Q_NULLPTR;
    const QPoint &cursor_pos = QCursor::pos();

    auto screens = qApp->screens();
    auto iter = std::find_if(screens.begin(), screens.end(), [cursor_pos](const QScreen *screen) {
        return screen->geometry().contains(cursor_pos);
    });

    if (iter != screens.end()) {
        cursor_screen = *iter;
    }

    if (!cursor_screen) {
        cursor_screen = qApp->primaryScreen();
    }

    int desktopWidth = cursor_screen->size().width();
    //    int desktopHeight = cursor_screen->size().height();//后面未用，注释掉
    int SpaceWidth = 20;
    int SpaceHeight = 70;
    int row, x, y;
    int numberPerRow = desktopWidth / (dialogWidth + SpaceWidth);
    Q_ASSERT(numberPerRow != 0);
    if (count % numberPerRow == 0) {
        row = count / numberPerRow;
    } else {
        row = count / numberPerRow + 1;
    }
    Q_UNUSED(row)
    int dialogsWidth;
    if (count / numberPerRow > 0) {
        dialogsWidth = dialogWidth * numberPerRow + SpaceWidth * (numberPerRow - 1);
    } else {
        dialogsWidth = dialogWidth * (count % numberPerRow) + SpaceWidth * (count % numberPerRow - 1);
    }

    //    int dialogsHeight = dialogHeight + SpaceHeight * (row - 1);//未用注释掉

    x = (desktopWidth - dialogsWidth) / 2 + (dialogWidth + SpaceWidth) * (index % numberPerRow);

    y = 5 + (index / numberPerRow) * SpaceHeight;
    return QPoint(x, y) + cursor_screen->geometry().topLeft();
}
