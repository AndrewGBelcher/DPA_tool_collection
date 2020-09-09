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

#include "cpadock.h"
#include "../sigsession.h"
#include "../device/devinst.h"
#include "../dialogs/dsmessagebox.h"

#include <QObject>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QRegExpValidator>
#include <QSplitter>
#include <QComboBox>

#include "libsigrok4DSL/libsigrok.h"

namespace pv {
namespace dock {


const int CPADock::MinTrigPosition = 1;


CPADock::CPADock(QWidget *parent, SigSession &session) :
    QScrollArea(parent),
    _session(session)
{
     int i;

    _widget = new QWidget(this);

    QFont font("Monaco");
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);


    com_port = new QComboBox(parent);
	com_port->addItem(tr("3"));
    com_port->addItem(tr("4"));
    com_port->addItem(tr("5"));
	com_port->setCurrentIndex(0);
   
    this->setWidget(_widget);
    //_widget->setGeometry(0, 0, sizeHint().width(), 1000);
    _widget->setObjectName("cpaWidget");
}

CPADock::~CPADock()
{
}

void CPADock::paintEvent(QPaintEvent *)
{
//    QStyleOption opt;
//    opt.init(this);
//    QPainter p(this);
//    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}




void CPADock::widget_enable(int index)
{
    (void) index;

    int i;
    int enable_stages;
    stages_label->setDisabled(false);
    stages_comboBox->setVisible(true);
    stages_comboBox->setDisabled(false);
    _adv_tabWidget->setDisabled(false);
    enable_stages = stages_comboBox->currentText().toInt();
    for (i = 0; i < enable_stages; i++) {
        stage_tabWidget->setTabEnabled(i, true);
    }
    for (i = enable_stages; i < TriggerStages; i++) {
          stage_tabWidget->setTabEnabled(i, false);
    }
}

void CPADock::value_changed()
{
/*
    QLineEdit* sc=dynamic_cast<QLineEdit*>(sender());
    if(sc != NULL)
        sc->setText(sc->text().toUpper());*/
}

void CPADock::device_updated()
{
    uint64_t hw_depth;
    bool stream = false;
    uint8_t maxRange;
    uint64_t sample_limits;
    GVariant *gvar = _session.get_device()->get_config(NULL, NULL, SR_CONF_HW_DEPTH);
    if (gvar != NULL) {
        hw_depth = g_variant_get_uint64(gvar);
        g_variant_unref(gvar);

        if (_session.get_device()->dev_inst()->mode == LOGIC) {

            gvar = _session.get_device()->get_config(NULL, NULL, SR_CONF_STREAM);
            if (gvar != NULL) {
                stream = g_variant_get_boolean(gvar);
                g_variant_unref(gvar);
            }

            sample_limits = _session.get_device()->get_sample_limit();
            if (stream)
                maxRange = 1;
            else if (hw_depth >= sample_limits)
                maxRange = DS_MAX_TRIG_PERCENT;
            else
                maxRange = ceil(hw_depth * DS_MAX_TRIG_PERCENT / sample_limits);
            //position_spinBox->setRange(MinTrigPosition, maxRange);
            //position_slider->setRange(MinTrigPosition, maxRange);

            if (_session.get_device()->name().contains("virtual") ||
                stream) {
                simple_radioButton->setChecked(true);
                //simple_trigger();
            }
        }
    }
}


void CPADock::init()
{
    // TRIGGERPOS
    //uint16_t pos = ds_trigger_get_pos();
    //position_slider->setValue(pos);
}




} // namespace dock
} // namespace pv
