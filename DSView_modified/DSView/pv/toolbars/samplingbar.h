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


#ifndef DSVIEW_PV_TOOLBARS_SAMPLINGBAR_H
#define DSVIEW_PV_TOOLBARS_SAMPLINGBAR_H

#include <stdint.h>
#include <list>
#include <map>

#include <boost/shared_ptr.hpp>

#include <QComboBox>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QMenu>

#include "../sigsession.h"
#include "../data/snapshot.h"
#include "../storesession.h"

#include <boost/thread.hpp>

struct st_dev_inst;
class QAction;

namespace pv {

class SigSession;
class StoreSession;

namespace device {
class DevInst;
}

namespace dialogs {
class deviceoptions;
class Calibration;
}

namespace toolbars {

class SamplingBar : public QToolBar
{
	Q_OBJECT

private:
    static const int ComboBoxMaxWidth = 200;
    static const int RefreshShort = 200;
    static const uint64_t LogicMaxSWDepth64 = SR_GB(16);
    static const uint64_t LogicMaxSWDepth32 = SR_GB(8);
    static const uint64_t AnalogMaxSWDepth = SR_Mn(100);
    static const QString RLEString;
    static const QString DIVString;

	boost::thread _thread;

public:
    SamplingBar(SigSession &session, QWidget *parent);

    void set_device_list(const std::list< boost::shared_ptr<pv::device::DevInst> > &devices,
                         boost::shared_ptr<pv::device::DevInst> selected);

    boost::shared_ptr<pv::device::DevInst> get_selected_device() const;

    void update_sample_rate_selector();

	void set_sampling(bool sampling);
    bool get_sampling() const;
    bool get_instant() const;





    void enable_toggle(bool enable);

	void uart();

    void enable_run_stop(bool enable);

    void enable_instant(bool enable);

    double hori_knob(int dir);
    double commit_hori_res();
    double get_hori_res();

public slots:
    void set_sample_rate(uint64_t sample_rate);

signals:
	void run_stop();
    void instant_stop();
	void commit_trigger(bool);
    void repeat_resume();
    void device_selected();
    void device_updated();
    void duration_changed();
    void show_calibration();
    void hide_calibration();

private:
	void update_sample_rate_selector_value();
    void update_sample_count_selector();
    void update_sample_count_selector_value();
    void commit_settings();
    void setting_adj();

	void capturing_thread_proc(QString );
	void cpa_thread_proc();
	void init_fpga_thread_proc();

private slots:
    void on_mode();
	void on_run_stop();

    void on_device_selected();
    void on_samplerate_sel(int index);
    void on_samplecount_sel(int index);

    void show_session_error(
     	const QString text, const QString info_text);

public slots:
    void on_configure();
    void zero_adj();
    void reload();
    void on_instant_stop();
	//void on_cpa_run();
	void on_cpa();
	void trig();
	void cpa_init();
private:
    SigSession &_session;


	std::unique_ptr<boost::thread> _capturing_thread;
	std::unique_ptr<boost::thread> _sampling_thread;
	std::unique_ptr<boost::thread> _cpa_thread;
	std::unique_ptr<boost::thread> _fpga_thread;

    mutable boost::recursive_mutex _sampling_mutex;
    bool _enable;
    bool _sampling;



    QComboBox _device_selector;
    std::map<const void*, boost::weak_ptr<device::DevInst> >
        _device_selector_map;
    bool _updating_device_selector;

    QToolButton _configure_button;

    QComboBox _sample_count;
    QComboBox _sample_rate;
    bool _updating_sample_rate;
    bool _updating_sample_count;

    QIcon _icon_stop;
    QIcon _icon_start;
    QIcon _icon_instant;
    QIcon _icon_cpa;
    QIcon _icon_start_dis;
    QIcon _icon_instant_dis;
    QIcon _icon_cpa_dis;
	QToolButton _run_stop_button;
    QToolButton _instant_button;
    QToolButton _trig_button;
    QToolButton _cpa_button;
    QAction* _run_stop_action;
    QAction* _instant_action;
    QAction* _trig_action;
    QAction* _cpa_action;

    QAction* _mode_action;
    QToolButton _mode_button;
    QMenu *_mode_menu;
    QAction *_action_repeat;
    QAction *_action_single;

    QIcon _icon_repeat;
    QIcon _icon_single;
    QIcon _icon_repeat_dis;
    QIcon _icon_single_dis;

    bool _instant;
    bool _cpa;
};

} // namespace toolbars
} // namespace pv

#endif // DSVIEW_PV_TOOLBARS_SAMPLINGBAR_H
