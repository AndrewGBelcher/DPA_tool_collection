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

#include "cpabar.h"
#include "../sigsession.h"
#include "../device/devinst.h"
#include "../dialogs/fftoptions.h"

#include <QApplication>

namespace pv {
namespace toolbars {

CPABar::CPABar(SigSession &session, QWidget *parent) :
    QToolBar("Cpa Bar", parent),
    _session(session),
    _enable(true),
    _cpa_button(this)
 
{
    setMovable(false);
    setContentsMargins(0,0,0,0);



    connect(&_cpa_button, SIGNAL(clicked()),
        this, SLOT(cpa_clicked()));
 

    _cpa_button.setIcon(QIcon::fromTheme("CPA",
        QIcon(":/icons/cpa.png")));
    _cpa_button.setCheckable(true);

    _cpa_action = addWidget(&_cpa_button);

}


void CPABar::cpa_clicked()
{
    //on_cpa(_cpa_button.isChecked());
	on_cpa(true);
}

void CPABar::update_cpa_btn(bool checked)
{
    //_cpa_button.setChecked(checked);
}

void CPABar::enable_toggle(bool enable)
{
    _cpa_button.setDisabled(!enable);


    _cpa_button.setIcon(enable ? QIcon::fromTheme("cpa", QIcon(":/icons/cpa.png")) :
                                  QIcon::fromTheme("cpa", QIcon(":/icons/cpa_dis.png")));
}



void CPABar::close_all()
{
    if (_cpa_button.isChecked()) {
        _cpa_button.setChecked(false);
        on_cpa(false);
    }

}

void CPABar::reload()
{
    close_all();
    if (_session.get_device()->dev_inst()->mode == LOGIC) {
        _cpa_action->setVisible(true);

    } else if (_session.get_device()->dev_inst()->mode == ANALOG) {
        _cpa_action->setVisible(false);

    } else if (_session.get_device()->dev_inst()->mode == DSO) {
        _cpa_action->setVisible(true);

    }
    enable_toggle(true);
    update();
}

void CPABar::on_actionFft_triggered()
{
    //pv::dialogs::FftOptions fft_dlg(this, _session);
  //  fft_dlg.exec();
}

} // namespace toolbars
} // namespace pv
