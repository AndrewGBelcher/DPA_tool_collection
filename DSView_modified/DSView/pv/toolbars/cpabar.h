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


#ifndef DSVIEW_PV_TOOLBARS_CPABAR_H
#define DSVIEW_PV_TOOLBARS_CPABAR_H

#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QMenu>

namespace pv {

class SigSession;

namespace toolbars {

class CPABar : public QToolBar
{
    Q_OBJECT
public:
    explicit CPABar(SigSession &session, QWidget *parent = 0);

    void enable_toggle(bool enable);
    void enable_protocol(bool enable);
    void close_all();
    void reload();

signals:
    void on_cpa(bool visible);
 

public slots:
    void cpa_clicked();

    void update_cpa_btn(bool checked);

    void on_actionFft_triggered();

private:
    SigSession& _session;
    bool _enable;
    QToolButton _cpa_button;

    QAction* _cpa_action;



};

} // namespace toolbars
} // namespace pv

#endif // DSVIEW_PV_TOOLBARS_TRIGBAR_H
