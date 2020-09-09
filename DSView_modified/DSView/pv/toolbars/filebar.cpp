/*
 * This file is part of the DSView project.
 * DSView is based on PulseView.
 *
 * Copyright (C) 2013 DreamSourceLab <support@dreamsourcelab.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <QMetaObject>
#include <QFileDialog>
#include <QApplication>

#include "filebar.h"
#include "../device/devinst.h"
#include "../dialogs/dsmessagebox.h"

#include <deque>

namespace pv {
namespace toolbars {

FileBar::FileBar(SigSession &session, QWidget *parent) :
    QToolBar("File Bar", parent),
    _enable(true),
    _session(session),
    _file_button(this)
{
    setMovable(false);

    _action_load = new QAction(this);
    _action_load->setText(QApplication::translate(
        "File", "&Load...", 0));
    _action_load->setIcon(QIcon::fromTheme("file",
        QIcon(":/icons/open.png")));
    _action_load->setObjectName(QString::fromUtf8("actionLoad"));
    connect(_action_load, SIGNAL(triggered()), this, SLOT(on_actionLoad_triggered()));

    _action_store = new QAction(this);
    _action_store->setText(QApplication::translate(
        "File", "S&tore...", 0));
    _action_store->setIcon(QIcon::fromTheme("file",
        QIcon(":/icons/save.png")));
    _action_store->setObjectName(QString::fromUtf8("actionStore"));
    connect(_action_store, SIGNAL(triggered()), this, SLOT(on_actionStore_triggered()));

    _action_default = new QAction(this);
    _action_default->setText(QApplication::translate(
        "File", "&Default...", 0));
    _action_default->setIcon(QIcon::fromTheme("file",
        QIcon(":/icons/gear.png")));
    _action_default->setObjectName(QString::fromUtf8("actionDefault"));
    connect(_action_default, SIGNAL(triggered()), this, SLOT(on_actionDefault_triggered()));

    _menu_session = new QMenu(tr("Settings"), parent);
    _menu_session->setIcon(QIcon::fromTheme("file",
        QIcon(":/icons/gear.png")));
    _menu_session->setObjectName(QString::fromUtf8("menuSession"));
    _menu_session->addAction(_action_load);
    _menu_session->addAction(_action_store);
    _menu_session->addAction(_action_default);

    _action_open = new QAction(this);
    _action_open->setText(QApplication::translate(
        "File", "&Open...", 0));
    _action_open->setIcon(QIcon::fromTheme("file",
        QIcon(":/icons/open.png")));
    _action_open->setObjectName(QString::fromUtf8("actionOpen"));
    connect(_action_open, SIGNAL(triggered()), this, SLOT(on_actionOpen_triggered()));

    _action_save = new QAction(this);
    _action_save->setText(QApplication::translate(
        "File", "&Save...", 0));
    _action_save->setIcon(QIcon::fromTheme("file",
        QIcon(":/icons/save.png")));
    _action_save->setObjectName(QString::fromUtf8("actionSave"));
    connect(_action_save, SIGNAL(triggered()), this, SIGNAL(on_save()));

    _action_export = new QAction(this);
    _action_export->setText(QApplication::translate("File", "&Export...", 0));
    _action_export->setIcon(QIcon::fromTheme("file",QIcon(":/icons/export.png")));
    _action_export->setObjectName(QString::fromUtf8("actionExport"));
    connect(_action_export, SIGNAL(triggered()), this, SIGNAL(on_export()));


    _action_capture = new QAction(this);
    _action_capture->setText(QApplication::translate(
        "File", "&Capture...", 0));
    _action_capture->setIcon(QIcon::fromTheme("file",
        QIcon(":/icons/capture.png")));
    _action_capture->setObjectName(QString::fromUtf8("actionCapture"));
    connect(_action_capture, SIGNAL(triggered()), this, SLOT(on_actionCapture_triggered()));

    _file_button.setPopupMode(QToolButton::InstantPopup);
    _file_button.setIcon(QIcon(":/icons/file.png"));

    _menu = new QMenu(this);
    _menu->addMenu(_menu_session);
    _menu->addAction(_action_open);
    _menu->addAction(_action_save);
    _menu->addAction(_action_export);
    _menu->addAction(_action_capture);
    _file_button.setMenu(_menu);
    addWidget(&_file_button);
}

void FileBar::on_actionOpen_triggered()
{
    const QString DIR_KEY("OpenPath");
    QSettings settings;
    // Show the dialog
    const QString file_name = QFileDialog::getOpenFileName(
        this, tr("Open File"), settings.value(DIR_KEY).toString(), tr(
            "DSView Data (*.dsl)"));
    if (!file_name.isEmpty()) {
        QDir CurrentDir;
        settings.setValue(DIR_KEY, CurrentDir.absoluteFilePath(file_name));
        load_file(file_name);
    }
}

void FileBar::session_error(
    const QString text, const QString info_text)
{
    QMetaObject::invokeMethod(this, "show_session_error",
        Qt::QueuedConnection, Q_ARG(QString, text),
        Q_ARG(QString, info_text));
}

void FileBar::show_session_error(
    const QString text, const QString info_text)
{
    dialogs::DSMessageBox msg(this);
    msg.mBox()->setText(text);
    msg.mBox()->setInformativeText(info_text);
    msg.mBox()->setStandardButtons(QMessageBox::Ok);
    msg.mBox()->setIcon(QMessageBox::Warning);
    msg.exec();
}

void FileBar::on_actionLoad_triggered()
{
    const QString DIR_KEY("SessionLoadPath");
    QSettings settings;
    // Show the dialog
    const QString file_name = QFileDialog::getOpenFileName(
        this, tr("Open Session"), settings.value(DIR_KEY).toString(), tr(
            "DSView Session (*.dsc)"));
    if (!file_name.isEmpty()) {
        QDir CurrentDir;
        settings.setValue(DIR_KEY, CurrentDir.absoluteFilePath(file_name));
        load_session(file_name);
    }
}

void FileBar::on_actionDefault_triggered()
{
    QDir dir(DS_RES_PATH);
    if (!dir.exists()) {
        dialogs::DSMessageBox msg(this);
        msg.mBox()->setText(tr("Session Load"));
        msg.mBox()->setInformativeText(tr("Cannot find default session file for this device!"));
        msg.mBox()->setStandardButtons(QMessageBox::Ok);
        msg.mBox()->setIcon(QMessageBox::Warning);
        msg.exec();
        return;
    }

    QString driver_name = _session.get_device()->name();
    QString mode_name = QString::number(_session.get_device()->dev_inst()->mode);
    QString file_name = dir.absolutePath() + "/" + driver_name + mode_name + ".def.dsc";
    if (!file_name.isEmpty())
        load_session(file_name);
}

void FileBar::on_actionStore_triggered()
{
    const QString DIR_KEY("SessionStorePath");
    QSettings settings;
    QString file_name = QFileDialog::getSaveFileName(
                this, tr("Save Session"), settings.value(DIR_KEY).toString(),
                tr("DSView Session (*.dsc)"));
    if (!file_name.isEmpty()) {
        QFileInfo f(file_name);
        if(f.suffix().compare("dsc"))
            file_name.append(tr(".dsc"));
        QDir CurrentDir;
        settings.setValue(DIR_KEY, CurrentDir.absoluteFilePath(file_name));
        store_session(file_name);
    }
}

void FileBar::on_actionCapture_triggered()
{
    _file_button.close();
    QCoreApplication::sendPostedEvents();
    QTimer::singleShot(100, this, SIGNAL(on_screenShot()));


}

void FileBar::enable_toggle(bool enable)
{
    _file_button.setDisabled(!enable);
    _file_button.setIcon(enable ? QIcon(":/icons/file.png") :
                                  QIcon(":/icons/file_dis.png"));
}

void FileBar::set_settings_en(bool enable)
{
    _menu_session->setDisabled(!enable);
}

} // namespace toolbars
} // namespace pv
