/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2014 Joel Holdsworth <joel@airwebreathe.org.uk>
 * Copyright (C) 2016 DreamSourceLab <support@dreamsourcelab.com>
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

/* __STDC_FORMAT_MACROS is required for PRIu64 and friends (in C++). */
#define __STDC_FORMAT_MACROS

#include "storesession.h"
#include "cpa.h"

#include <pv/sigsession.h>
#include <pv/data/logic.h>
#include <pv/data/logicsnapshot.h>
#include <pv/data/dsosnapshot.h>
#include <pv/data/analogsnapshot.h>
#include <pv/data/decoderstack.h>
#include <pv/data/decode/decoder.h>
#include <pv/data/decode/row.h>
#include <pv/view/trace.h>
#include <pv/view/signal.h>
#include <pv/view/logicsignal.h>
#include <pv/view/dsosignal.h>
#include <pv/view/decodetrace.h>
#include <pv/device/devinst.h>
#include <pv/dock/protocoldock.h>

#include <boost/foreach.hpp>

#include <QFileDialog>

using boost::dynamic_pointer_cast;
using boost::mutex;
using boost::shared_ptr;
using boost::thread;
using boost::lock_guard;
using std::deque;
using std::make_pair;
using std::min;
using std::pair;
using std::set;
using std::string;
using std::vector;

namespace pv {

StoreSession::StoreSession(SigSession &session) :
	_session(session),
    _outModule(NULL),
	_units_stored(0),
    _unit_count(0),
    _has_error(false)
{
}

StoreSession::~StoreSession()
{
	wait();
}

pair<uint64_t, uint64_t> StoreSession::progress() const
{
    //lock_guard<mutex> lock(_mutex);
	return make_pair(_units_stored, _unit_count);
}

const QString& StoreSession::error() const
{
    //lock_guard<mutex> lock(_mutex);
	return _error;
}

void StoreSession::wait()
{
    if (_thread.joinable())
        _thread.join();
}

void StoreSession::cancel()
{
    _thread.interrupt();
}

QList<QString> StoreSession::getSuportedExportFormats(){
    const struct sr_output_module** supportedModules = sr_output_list();
    QList<QString> list;
    while(*supportedModules){
        if(*supportedModules == NULL)
            break;
        if (_session.get_device()->dev_inst()->mode != LOGIC &&
            strcmp((*supportedModules)->id, "csv"))
            break;
        QString format((*supportedModules)->desc);
        format.append(" (*.");
        format.append((*supportedModules)->id);
        format.append(")");
        list.append(format);
        supportedModules++;
    }
    return list;
}

bool StoreSession::save_start()
{
    std::set<int> type_set;
    BOOST_FOREACH(const boost::shared_ptr<view::Signal> sig, _session.get_signals()) {
        assert(sig);
        type_set.insert(sig->get_type());
    }

    if (type_set.size() > 1) {
        _error = tr("DSView does not currently support"
                    "file saving for multiple data types.");
        return false;
    } else if (type_set.size() == 0) {
        _error = tr("No data to save.");
        return false;
    }

    const boost::shared_ptr<data::Snapshot> snapshot(_session.get_snapshot(*type_set.begin()));
	assert(snapshot);
    // Check we have data
    if (snapshot->empty()) {
        _error = tr("No data to save.");
        return false;
    }

    const QString DIR_KEY("SavePath");
    QSettings settings;

    // Show the dialog
    _file_name = QFileDialog::getSaveFileName(
                    NULL, tr("Save File"), settings.value(DIR_KEY).toString(),
                    tr("DSView Data (*.dsl)"));

    if (!_file_name.isEmpty()) {
        QFileInfo f(_file_name);
        if(f.suffix().compare("dsl"))
            _file_name.append(tr(".dsl"));
        QDir CurrentDir;
        settings.setValue(DIR_KEY, CurrentDir.absoluteFilePath(_file_name));

        QString meta_file = meta_gen(snapshot);
    #ifdef ENABLE_DECODE
        QString decoders_file = decoders_gen();
    #else
        QString decoders_file = NULL;
    #endif
        if (meta_file == NULL) {
            _error = tr("Generate temp file failed.");
            return false;
        } else {
            int ret = sr_session_save_init(_file_name.toLocal8Bit().data(),
                                 meta_file.toLocal8Bit().data(),
                                 decoders_file.toLocal8Bit().data());
            if (ret != SR_OK) {
                _error = tr("Failed to create zip file. Please check write permission of this path.");
                return false;
            } else {
                _thread = boost::thread(&StoreSession::save_proc, this, snapshot);
                return !_has_error;
            }
        }
    }

    _error.clear();
    return false;
}

void StoreSession::save_proc(shared_ptr<data::Snapshot> snapshot)
{
	assert(snapshot);

    shared_ptr<data::LogicSnapshot> logic_snapshot;
    shared_ptr<data::AnalogSnapshot> analog_snapshot;
    shared_ptr<data::DsoSnapshot> dso_snapshot;

    if ((logic_snapshot = boost::dynamic_pointer_cast<data::LogicSnapshot>(snapshot))) {
        uint16_t to_save_probes = 0;
        BOOST_FOREACH(const boost::shared_ptr<view::Signal> s, _session.get_signals()) {
            if (s->enabled() && logic_snapshot->has_data(s->get_index()))
                to_save_probes++;
        }
        _unit_count = logic_snapshot->get_sample_count() / 8 * to_save_probes;
        int num = logic_snapshot->get_block_num();
        bool sample;

        BOOST_FOREACH(const boost::shared_ptr<view::Signal> s, _session.get_signals()) {
            int ch_type = s->get_type();
            if (ch_type == SR_CHANNEL_LOGIC) {
                int ch_index = s->get_index();
                if (!s->enabled() || !logic_snapshot->has_data(ch_index))
                    continue;
                for (int i = 0; !boost::this_thread::interruption_requested() && i < num; i++) {
                    uint8_t *buf = logic_snapshot->get_block_buf(i, ch_index, sample);
                    uint64_t size = logic_snapshot->get_block_size(i);
                    bool need_malloc = (buf == NULL);
                    if (need_malloc) {
                        buf = (uint8_t *)malloc(size);
                        if (buf == NULL) {
                            _has_error = true;
                            _error = tr("Malloc failed.");
                            return;
                        }
                        memset(buf, sample ? 0xff : 0x0, size);
                    }
                    sr_session_append(_file_name.toLocal8Bit().data(), buf, size,
                                      i, ch_index, ch_type, File_Version);
                    _units_stored += size;
                    if (need_malloc)
                        free(buf);
                    progress_updated();
                }
            }
        }
    } else {
        int ch_type = -1;
        BOOST_FOREACH(const boost::shared_ptr<view::Signal> s, _session.get_signals()) {
            ch_type = s->get_type();
            break;
        }
        if (ch_type != -1) {
            const int num = snapshot->get_block_num();
            _unit_count = snapshot->get_sample_count() *
                          snapshot->get_unit_bytes() *
                          snapshot->get_channel_num();
            uint8_t *buf = (uint8_t *)snapshot->get_data() +
                           (snapshot->get_ring_start() * snapshot->get_unit_bytes() * snapshot->get_channel_num());
            const uint8_t *buf_start = (uint8_t *)snapshot->get_data();
            const uint8_t *buf_end = buf_start + _unit_count;

            for (int i = 0; !boost::this_thread::interruption_requested() && i < num; i++) {
                const uint64_t size = snapshot->get_block_size(i);
                if ((buf + size) > buf_end) {
                    uint8_t *tmp = (uint8_t *)malloc(size);
                    if (tmp == NULL) {
                        _has_error = true;
                        _error = tr("Malloc failed.");
                        return;
                    }
                    memcpy(tmp, buf, buf_end-buf);
                    memcpy(tmp+(buf_end-buf), buf_start, buf+size-buf_end);
                    sr_session_append(_file_name.toLocal8Bit().data(), tmp, size,
                                      i, 0, ch_type, File_Version);
                    buf += (size - _unit_count);
                    free(tmp);
                } else {
                    sr_session_append(_file_name.toLocal8Bit().data(), buf, size,
                                      i, 0, ch_type, File_Version);
                    buf += size;
                }
                _units_stored += size;
                progress_updated();
            }
        }
    }

	progress_updated();
}

QString StoreSession::meta_gen(boost::shared_ptr<data::Snapshot> snapshot)
{
    GSList *l;
    GVariant *gvar;
    FILE *meta = NULL;
    struct sr_channel *probe;
    int probecnt;
    char *s;
    struct sr_status status;
    QString metafile;

    /* init "metadata" */
    QDir dir;
    #if QT_VERSION >= 0x050400
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    #else
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    #endif
    if(dir.mkpath(path)) {
        dir.cd(path);
        metafile = dir.absolutePath() + "/DSView-meta-XXXXXX";
    } else {
        return NULL;
    }

    const sr_dev_inst *sdi = _session.get_device()->dev_inst();
    meta = fopen(metafile.toLocal8Bit().data(), "wb");
    if (meta == NULL) {
        qDebug() << "Failed to create temp meta file.";
        return NULL;
    }

    fprintf(meta, "[version]\n");
//    if (sdi->mode != LOGIC)
//        fprintf(meta, "version = %d\n", 1); // should be updated in next version
//    else
//        fprintf(meta, "version = %d\n", File_Version);
    fprintf(meta, "version = %d\n", File_Version);

    /* metadata */

    fprintf(meta, "[header]\n");
    if (sdi->driver) {
        fprintf(meta, "driver = %s\n", sdi->driver->name);
        fprintf(meta, "device mode = %d\n", sdi->mode);
    }

    /* metadata */
    fprintf(meta, "capturefile = data\n");
    fprintf(meta, "total samples = %" PRIu64 "\n", snapshot->get_sample_count());

    if (sdi->mode != LOGIC) {
        fprintf(meta, "total probes = %d\n", snapshot->get_channel_num());
        fprintf(meta, "total blocks = %d\n", snapshot->get_block_num());
    }

    shared_ptr<data::LogicSnapshot> logic_snapshot;
    if ((logic_snapshot = dynamic_pointer_cast<data::LogicSnapshot>(snapshot))) {
        uint16_t to_save_probes = 0;
        for (l = sdi->channels; l; l = l->next) {
            probe = (struct sr_channel *)l->data;
            if (probe->enabled && logic_snapshot->has_data(probe->index))
                to_save_probes++;
        }
        fprintf(meta, "total probes = %d\n", to_save_probes);
        fprintf(meta, "total blocks = %d\n", logic_snapshot->get_block_num());
    }

    s = sr_samplerate_string(_session.cur_samplerate());
    fprintf(meta, "samplerate = %s\n", s);

    if (sdi->mode == DSO) {
        gvar = _session.get_device()->get_config(NULL, NULL, SR_CONF_TIMEBASE);
        if (gvar != NULL) {
            uint64_t tmp_u64 = g_variant_get_uint64(gvar);
            fprintf(meta, "hDiv = %" PRIu64 "\n", tmp_u64);
            g_variant_unref(gvar);
        }
        gvar = _session.get_device()->get_config(NULL, NULL, SR_CONF_UNIT_BITS);
        if (gvar != NULL) {
            uint8_t tmp_u8 = g_variant_get_byte(gvar);
            fprintf(meta, "bits = %d\n", tmp_u8);
            g_variant_unref(gvar);
        }
    } else if (sdi->mode == LOGIC) {
        fprintf(meta, "trigger time = %lld\n", _session.get_trigger_time().toMSecsSinceEpoch());
    } else if (sdi->mode == ANALOG) {
        shared_ptr<data::AnalogSnapshot> analog_snapshot;
        if ((analog_snapshot = dynamic_pointer_cast<data::AnalogSnapshot>(snapshot))) {
            uint8_t tmp_u8 = analog_snapshot->get_unit_bytes();
            fprintf(meta, "bits = %d\n", tmp_u8*8);
        }
    }
    fprintf(meta, "trigger pos = %" PRIu64 "\n", _session.get_trigger_pos());

    probecnt = 0;
    for (l = sdi->channels; l; l = l->next) {
        probe = (struct sr_channel *)l->data;
        if (snapshot->has_data(probe->index)) {
            if (probe->name)
                fprintf(meta, "probe%d = %s\n", (sdi->mode == LOGIC) ? probe->index : probecnt, probe->name);
            if (probe->trigger)
                fprintf(meta, " trigger%d = %s\n", probecnt, probe->trigger);
            if (sdi->mode == DSO) {
                fprintf(meta, " enable%d = %d\n", probecnt, probe->enabled);
                fprintf(meta, " coupling%d = %d\n", probecnt, probe->coupling);
                fprintf(meta, " vDiv%d = %" PRIu64 "\n", probecnt, probe->vdiv);
                fprintf(meta, " vFactor%d = %d\n", probecnt, probe->vfactor);
                fprintf(meta, " vPos%d = %lf\n", probecnt, probe->vpos);
                fprintf(meta, " vTrig%d = %d\n", probecnt, probe->trig_value);
                if (sr_status_get(sdi, &status, false, 0, 0) == SR_OK) {
                    if (probe->index == 0) {
                        fprintf(meta, " period%d = %" PRIu64 "\n", probecnt, status.ch0_period);
                        fprintf(meta, " pcnt%d = %" PRIu32 "\n", probecnt, status.ch0_pcnt);
                        fprintf(meta, " max%d = %d\n", probecnt, status.ch0_max);
                        fprintf(meta, " min%d = %d\n", probecnt, status.ch0_min);
                    } else {
                        fprintf(meta, " period%d = %" PRIu64 "\n", probecnt, status.ch1_period);
                        fprintf(meta, " pcnt%d = %" PRIu32 "\n", probecnt, status.ch1_pcnt);
                        fprintf(meta, " max%d = %d\n", probecnt, status.ch1_max);
                        fprintf(meta, " min%d = %d\n", probecnt, status.ch1_min);
                    }
                }
            } else if (sdi->mode == ANALOG) {
                fprintf(meta, " enable%d = %d\n", probecnt, probe->enabled);
                fprintf(meta, " coupling%d = %d\n", probecnt, probe->coupling);
                fprintf(meta, " vDiv%d = %" PRIu64 "\n", probecnt, probe->vdiv);
                fprintf(meta, " vPos%d = %lf\n", probecnt, probe->vpos);
                fprintf(meta, " mapUnit%d = %s\n", probecnt, probe->map_unit);
                fprintf(meta, " mapMax%d = %lf\n", probecnt, probe->map_max);
                fprintf(meta, " mapMin%d = %lf\n", probecnt, probe->map_min);
            }
            probecnt++;
        }
    }

    fclose(meta);

    return metafile;
}

bool StoreSession::export_start()
{
    std::set<int> type_set;
    BOOST_FOREACH(const boost::shared_ptr<view::Signal> sig, _session.get_signals()) {
        assert(sig);
        type_set.insert(sig->get_type());
    }

    if (type_set.size() > 1) {
        _error = tr("DSView does not currently support"
                    "file export for multiple data types.");
        return false;
    } else if (type_set.size() == 0) {
        _error = tr("No data to save.");
        return false;
    }

    const boost::shared_ptr<data::Snapshot> snapshot(_session.get_snapshot(*type_set.begin()));
    assert(snapshot);
    // Check we have data
    if (snapshot->empty()) {
        _error = tr("No data to save.");
        return false;
    }

    const QString DIR_KEY("ExportPath");
    QSettings settings;

    // Show the dialog
    QList<QString> supportedFormats = getSuportedExportFormats();
    QString filter;
    for(int i = 0; i < supportedFormats.count();i++){
        filter.append(supportedFormats[i]);
        if(i < supportedFormats.count() - 1)
            filter.append(";;");
    }
    _file_name = QFileDialog::getSaveFileName(
                NULL, tr("Export Data"), settings.value(DIR_KEY).toString(),filter,&filter);
    if (!_file_name.isEmpty()) {
        QFileInfo f(_file_name);
        QStringList list = filter.split('.').last().split(')');
        _suffix = list.first();
        if(f.suffix().compare(_suffix))
            _file_name += tr(".") + _suffix;
        QDir CurrentDir;
        settings.setValue(DIR_KEY, CurrentDir.absoluteFilePath(_file_name));

        const struct sr_output_module** supportedModules = sr_output_list();
        while(*supportedModules){
            if(*supportedModules == NULL)
                break;
            if(!strcmp((*supportedModules)->id, _suffix.toLocal8Bit().data())){
                _outModule = *supportedModules;
                break;
            }
            supportedModules++;
        }

        if(_outModule == NULL) {
            _error = tr("Invalid export format.");
        } else {
            _thread = boost::thread(&StoreSession::export_proc, this, snapshot);
            return !_has_error;
        }
    }

    _error.clear();
    return false;
}


bool StoreSession::export_cpa_start(QString file_name)
{

 std::set<int> type_set;

    const boost::shared_ptr<data::Snapshot> snapshot(_session.get_snapshot(SR_CHANNEL_DSO));
assert(snapshot);

    // Check we have data
	//fprintf(stdout, "after snapshot, export cpa start\n");

	snapshot->set_exporting_status(false);
	//qDebug("were in export, filename:%s", qUtf8Printable(file_name));
_file_name = file_name;

    //std::set<int> type_set;
    BOOST_FOREACH(const boost::shared_ptr<view::Signal> sig, _session.get_signals()) {
        assert(sig);
        type_set.insert(sig->get_type());
    }

    if (type_set.size() > 1) {
        _error = tr("DSView does not currently support"
                    "file export for multiple data types.");
        return false;
    } else if (type_set.size() == 0) {
	//	fprintf(stdout, "no data to save, size\n");
        _error = tr("No data to save.");
        return false;
    }



	fprintf(stdout, "before checkempty, export cpa start\n");

    // Check we have data
    if (snapshot->empty()) {
        _error = tr("No data to save.");
		//fprintf(stdout, "no data to save, empty\n");
        return false;
    }

//	fprintf(stdout, "after checkempty, export cpa start\n");

    const QString DIR_KEY("ExportPath");
    QSettings settings;

    // Show the dialog
    QList<QString> supportedFormats = getSuportedExportFormats();
    QString filter;
    for(int i = 0; i < supportedFormats.count();i++){
        filter.append(supportedFormats[i]);
        if(i < supportedFormats.count() - 1)
            filter.append(";;");
    }

   // _file_name = QFileDialog::getSaveFileName(
     //           NULL, tr("Export Data"), "./cpatest.csv",filter,&filter);

	//bool sn = snapshot->empty();
	//while(sn);




    if (!_file_name.isEmpty()) {
        QFileInfo f(_file_name);
        QStringList list = filter.split('.').last().split(')');
        _suffix = list.first();
        if(f.suffix().compare(_suffix))
            _file_name += tr(".") + _suffix;
        QDir CurrentDir;
        settings.setValue(DIR_KEY, CurrentDir.absoluteFilePath(_file_name));

        const struct sr_output_module** supportedModules = sr_output_list();
        while(*supportedModules){
            if(*supportedModules == NULL)
                break;
            if(!strcmp((*supportedModules)->id, _suffix.toLocal8Bit().data())){
                _outModule = *supportedModules;
                break;
            }
            supportedModules++;
        }

        if(_outModule == NULL) {
            _error = tr("Invalid export format.");
        } else {
            _thread = boost::thread(&StoreSession::export_proc_cpa, this, snapshot);
            return !_has_error;
        }
    }

    _error.clear();
    return false;
}



void StoreSession::export_proc_cpa(shared_ptr<data::Snapshot> snapshot)
{

	snapshot->set_exporting_status(false);

    assert(snapshot);

    shared_ptr<data::LogicSnapshot> logic_snapshot;
    shared_ptr<data::AnalogSnapshot> analog_snapshot;
    shared_ptr<data::DsoSnapshot> dso_snapshot;
    int channel_type;

    if ((logic_snapshot = boost::dynamic_pointer_cast<data::LogicSnapshot>(snapshot))) {
        channel_type = SR_CHANNEL_LOGIC;
    } else if ((dso_snapshot = boost::dynamic_pointer_cast<data::DsoSnapshot>(snapshot))) {
        channel_type = SR_CHANNEL_DSO;
    } else if ((analog_snapshot = boost::dynamic_pointer_cast<data::AnalogSnapshot>(snapshot))) {
        channel_type = SR_CHANNEL_ANALOG;
    } else {
        _has_error = true;
        _error = tr("data type don't support.");
        return;
    }

    GHashTable *params = g_hash_table_new(g_str_hash, g_str_equal);
    GVariant* filenameGVariant = g_variant_new_bytestring(_file_name.toLocal8Bit().data());
    g_hash_table_insert(params, (char*)"filename", filenameGVariant);
    GVariant* typeGVariant = g_variant_new_int16(channel_type);
    g_hash_table_insert(params, (char*)"type", typeGVariant);

    struct sr_output output;
    output.module = (sr_output_module*) _outModule;
    output.sdi = _session.get_device()->dev_inst();
    output.param = NULL;
    if(_outModule->init)
        _outModule->init(&output, params);
    QFile file(_file_name);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);

    // Meta
    GString *data_out;
    struct sr_datafeed_packet p;
    struct sr_datafeed_meta meta;
    struct sr_config *src;
    src = sr_config_new(SR_CONF_SAMPLERATE,
            g_variant_new_uint64(_session.cur_samplerate()));
    meta.config = g_slist_append(NULL, src);
    src = sr_config_new(SR_CONF_LIMIT_SAMPLES,
            g_variant_new_uint64(snapshot->get_sample_count()));
    meta.config = g_slist_append(meta.config, src);
    p.type = SR_DF_META;
    p.status = SR_PKT_OK;
    p.payload = &meta;
    _outModule->receive(&output, &p, &data_out);
    if(data_out){
        out << QString::fromUtf8((char*) data_out->str);
        g_string_free(data_out,TRUE);
    }
    for (GSList *l = meta.config; l; l = l->next) {
        src = (struct sr_config *)l->data;
        sr_config_free(src);
    }
    g_slist_free(meta.config);

    if (channel_type == SR_CHANNEL_LOGIC) {
        _unit_count = logic_snapshot->get_sample_count();
        int blk_num = logic_snapshot->get_block_num();
        bool sample;
        std::vector<uint8_t *> buf_vec;
        std::vector<bool> buf_sample;
        for (int blk = 0; !boost::this_thread::interruption_requested()  &&
                          blk < blk_num; blk++) {
            uint64_t buf_sample_num = logic_snapshot->get_block_size(blk) * 8;
            buf_vec.clear();
            buf_sample.clear();
            BOOST_FOREACH(const boost::shared_ptr<view::Signal> s, _session.get_signals()) {
                int ch_type = s->get_type();
                if (ch_type == SR_CHANNEL_LOGIC) {
                    int ch_index = s->get_index();
                    if (!logic_snapshot->has_data(ch_index))
                        continue;
                    uint8_t *buf = logic_snapshot->get_block_buf(blk, ch_index, sample);
                    buf_vec.push_back(buf);
                    buf_sample.push_back(sample);
                }
            }

            uint16_t unitsize = ceil(buf_vec.size() / 8.0);
            unsigned int usize = 8192;
            unsigned int size = usize;
            struct sr_datafeed_logic lp;
            for(uint64_t i = 0; !boost::this_thread::interruption_requested() &&
                                i < buf_sample_num; i+=usize){
                if(buf_sample_num - i < usize)
                    size = buf_sample_num - i;
                uint8_t *xbuf = (uint8_t *)malloc(size * unitsize);
                if (xbuf == NULL) {
                    _has_error = true;
                    _error = tr("xbuffer malloc failed.");
                    return;
                }
                memset(xbuf, 0, size * unitsize);
                for (uint64_t j = 0; j < size; j++) {
                    for (unsigned int k = 0; k < buf_vec.size(); k++) {
                        if (buf_vec[k] == NULL && buf_sample[k])
                            xbuf[j*unitsize+k/8] +=  1 << k%8;
                        else if (buf_vec[k] && (buf_vec[k][(i+j)/8] & (1 << j%8)))
                            xbuf[j*unitsize+k/8] +=  1 << k%8;
                    }
                }
                lp.data = xbuf;
                lp.length = size * unitsize;
                lp.unitsize = unitsize;
                p.type = SR_DF_LOGIC;
                p.status = SR_PKT_OK;
                p.payload = &lp;
                _outModule->receive(&output, &p, &data_out);
                if(data_out){
                    out << QString::fromUtf8((char*) data_out->str);
                    g_string_free(data_out,TRUE);
                }

                _units_stored += size;
                if (xbuf)
                    free(xbuf);
                progress_updated();
            }
        }
    } 


	else if (channel_type == SR_CHANNEL_DSO) {
        _unit_count = snapshot->get_sample_count();
        unsigned char* datat = (unsigned char*)snapshot->get_data();
        unsigned int usize = 8192;
        unsigned int size = usize;
        struct sr_datafeed_dso dp;
        for(uint64_t i = 0; !boost::this_thread::interruption_requested() && i < _unit_count; i+=usize){
            if(_unit_count - i < usize)
                size = _unit_count - i;
            dp.data = &datat[i*snapshot->get_channel_num()];
            dp.num_samples = size;
            p.type = SR_DF_DSO;
            p.status = SR_PKT_OK;
            p.payload = &dp;

			if(i > CPA_SAMPLE_COUNT_START && i <= CPA_SAMPLE_COUNT_END)
           	{
				_outModule->receive(&output, &p, &data_out);
			}
            
			if(data_out){
                out << (char*) data_out->str;
                g_string_free(data_out,TRUE);
            }

            _units_stored += size;
            progress_updated();
        }
		
    }

	 else if (channel_type == SR_CHANNEL_ANALOG) {
        _unit_count = snapshot->get_sample_count();
        unsigned char* datat = (unsigned char*)snapshot->get_data();
        unsigned int usize = 8192;
        unsigned int size = usize;
        struct sr_datafeed_analog ap;
        for(uint64_t i = 0; !boost::this_thread::interruption_requested() && i < _unit_count; i+=usize){

            if(_unit_count - i < usize)
                size = _unit_count - i;
            ap.data = &datat[i*snapshot->get_channel_num()];
            ap.num_samples = size;
            p.type = SR_DF_ANALOG;
            p.status = SR_PKT_OK;
            p.payload = &ap;
            _outModule->receive(&output, &p, &data_out);
            if(data_out){
                out << (char*) data_out->str;
                g_string_free(data_out,TRUE);
            }

            _units_stored += size;
            progress_updated();
        }
    }

    // optional, as QFile destructor will already do it:
    file.close();
    _outModule->cleanup(&output);
    g_hash_table_destroy(params);
    g_variant_unref(filenameGVariant);

    progress_updated();
	snapshot->set_exporting_status(true);
}


void StoreSession::export_proc(shared_ptr<data::Snapshot> snapshot)
{

	snapshot->set_exporting_status(false);

    assert(snapshot);

    shared_ptr<data::LogicSnapshot> logic_snapshot;
    shared_ptr<data::AnalogSnapshot> analog_snapshot;
    shared_ptr<data::DsoSnapshot> dso_snapshot;
    int channel_type;

    if ((logic_snapshot = boost::dynamic_pointer_cast<data::LogicSnapshot>(snapshot))) {
        channel_type = SR_CHANNEL_LOGIC;
    } else if ((dso_snapshot = boost::dynamic_pointer_cast<data::DsoSnapshot>(snapshot))) {
        channel_type = SR_CHANNEL_DSO;
    } else if ((analog_snapshot = boost::dynamic_pointer_cast<data::AnalogSnapshot>(snapshot))) {
        channel_type = SR_CHANNEL_ANALOG;
    } else {
        _has_error = true;
        _error = tr("data type don't support.");
        return;
    }

    GHashTable *params = g_hash_table_new(g_str_hash, g_str_equal);
    GVariant* filenameGVariant = g_variant_new_bytestring(_file_name.toLocal8Bit().data());
    g_hash_table_insert(params, (char*)"filename", filenameGVariant);
    GVariant* typeGVariant = g_variant_new_int16(channel_type);
    g_hash_table_insert(params, (char*)"type", typeGVariant);

    struct sr_output output;
    output.module = (sr_output_module*) _outModule;
    output.sdi = _session.get_device()->dev_inst();
    output.param = NULL;
    if(_outModule->init)
        _outModule->init(&output, params);
    QFile file(_file_name);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);

    // Meta
    GString *data_out;
    struct sr_datafeed_packet p;
    struct sr_datafeed_meta meta;
    struct sr_config *src;
    src = sr_config_new(SR_CONF_SAMPLERATE,
            g_variant_new_uint64(_session.cur_samplerate()));
    meta.config = g_slist_append(NULL, src);
    src = sr_config_new(SR_CONF_LIMIT_SAMPLES,
            g_variant_new_uint64(snapshot->get_sample_count()));
    meta.config = g_slist_append(meta.config, src);
    p.type = SR_DF_META;
    p.status = SR_PKT_OK;
    p.payload = &meta;
    _outModule->receive(&output, &p, &data_out);
    if(data_out){
        out << QString::fromUtf8((char*) data_out->str);
        g_string_free(data_out,TRUE);
    }
    for (GSList *l = meta.config; l; l = l->next) {
        src = (struct sr_config *)l->data;
        sr_config_free(src);
    }
    g_slist_free(meta.config);

    if (channel_type == SR_CHANNEL_LOGIC) {
        _unit_count = logic_snapshot->get_sample_count();
        int blk_num = logic_snapshot->get_block_num();
        bool sample;
        std::vector<uint8_t *> buf_vec;
        std::vector<bool> buf_sample;
        for (int blk = 0; !boost::this_thread::interruption_requested()  &&
                          blk < blk_num; blk++) {
            uint64_t buf_sample_num = logic_snapshot->get_block_size(blk) * 8;
            buf_vec.clear();
            buf_sample.clear();
            BOOST_FOREACH(const boost::shared_ptr<view::Signal> s, _session.get_signals()) {
                int ch_type = s->get_type();
                if (ch_type == SR_CHANNEL_LOGIC) {
                    int ch_index = s->get_index();
                    if (!logic_snapshot->has_data(ch_index))
                        continue;
                    uint8_t *buf = logic_snapshot->get_block_buf(blk, ch_index, sample);
                    buf_vec.push_back(buf);
                    buf_sample.push_back(sample);
                }
            }

            uint16_t unitsize = ceil(buf_vec.size() / 8.0);
            unsigned int usize = 8192;
            unsigned int size = usize;
            struct sr_datafeed_logic lp;
            for(uint64_t i = 0; !boost::this_thread::interruption_requested() &&
                                i < buf_sample_num; i+=usize){
                if(buf_sample_num - i < usize)
                    size = buf_sample_num - i;
                uint8_t *xbuf = (uint8_t *)malloc(size * unitsize);
                if (xbuf == NULL) {
                    _has_error = true;
                    _error = tr("xbuffer malloc failed.");
                    return;
                }
                memset(xbuf, 0, size * unitsize);
                for (uint64_t j = 0; j < size; j++) {
                    for (unsigned int k = 0; k < buf_vec.size(); k++) {
                        if (buf_vec[k] == NULL && buf_sample[k])
                            xbuf[j*unitsize+k/8] +=  1 << k%8;
                        else if (buf_vec[k] && (buf_vec[k][(i+j)/8] & (1 << j%8)))
                            xbuf[j*unitsize+k/8] +=  1 << k%8;
                    }
                }
                lp.data = xbuf;
                lp.length = size * unitsize;
                lp.unitsize = unitsize;
                p.type = SR_DF_LOGIC;
                p.status = SR_PKT_OK;
                p.payload = &lp;
                _outModule->receive(&output, &p, &data_out);
                if(data_out){
                    out << QString::fromUtf8((char*) data_out->str);
                    g_string_free(data_out,TRUE);
                }

                _units_stored += size;
                if (xbuf)
                    free(xbuf);
                progress_updated();
            }
        }
    } 
	
	// modified to save only select range
	else if (channel_type == SR_CHANNEL_DSO) {
        _unit_count = snapshot->get_sample_count();
        unsigned char* datat = (unsigned char*)snapshot->get_data();
        unsigned int usize = 8192;
        unsigned int size = usize;
        struct sr_datafeed_dso dp;
        for(uint64_t i = 0; !boost::this_thread::interruption_requested() && i < _unit_count; i+=usize){
            if(_unit_count - i < usize)
                size = _unit_count - i;
            dp.data = &datat[i*snapshot->get_channel_num()];
            dp.num_samples = size;
            p.type = SR_DF_DSO;
            p.status = SR_PKT_OK;
            p.payload = &dp;

			_outModule->receive(&output, &p, &data_out);
	
			if(data_out){
                out << (char*) data_out->str;
                g_string_free(data_out,TRUE);
            }

            _units_stored += size;
            progress_updated();
        }
    } 

	else if (channel_type == SR_CHANNEL_ANALOG) {
        _unit_count = snapshot->get_sample_count();
        unsigned char* datat = (unsigned char*)snapshot->get_data();
        unsigned int usize = 8192;
        unsigned int size = usize;
        struct sr_datafeed_analog ap;
        for(uint64_t i = 0; !boost::this_thread::interruption_requested() && i < _unit_count; i+=usize){

            if(_unit_count - i < usize)
                size = _unit_count - i;
            ap.data = &datat[i*snapshot->get_channel_num()];
            ap.num_samples = size;
            p.type = SR_DF_ANALOG;
            p.status = SR_PKT_OK;
            p.payload = &ap;
            _outModule->receive(&output, &p, &data_out);
            if(data_out){
                out << (char*) data_out->str;
                g_string_free(data_out,TRUE);
            }

            _units_stored += size;
            progress_updated();
        }
    }

    // optional, as QFile destructor will already do it:
    file.close();
    _outModule->cleanup(&output);
    g_hash_table_destroy(params);
    g_variant_unref(filenameGVariant);

    progress_updated();
	snapshot->set_exporting_status(true);
}

#ifdef ENABLE_DECODE
QString StoreSession::decoders_gen()
{
    QDir dir;
    #if QT_VERSION >= 0x050400
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    #else
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    #endif
    if(dir.mkpath(path)) {
        dir.cd(path);

        QString file_name = dir.absolutePath() + "/DSView-decoders-XXXXXX";
        QFile sessionFile(file_name);
        if (!sessionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug("Warning: Couldn't open session file to write!");
            return NULL;
        }
        QTextStream outStream(&sessionFile);
        outStream.setCodec("UTF-8");
        outStream.setGenerateByteOrderMark(true);

        QJsonArray dec_array = json_decoders();
        QJsonDocument sessionDoc(dec_array);
        outStream << QString::fromUtf8(sessionDoc.toJson());
        sessionFile.close();

        return file_name;
    } else {
        return NULL;
    }

}

QJsonArray StoreSession::json_decoders()
{
    QJsonArray dec_array;
    BOOST_FOREACH(boost::shared_ptr<view::DecodeTrace> t, _session.get_decode_signals()) {
        QJsonObject dec_obj;
        QJsonArray stack_array;
        QJsonObject show_obj;
        const boost::shared_ptr<data::DecoderStack>& stack = t->decoder();
        const std::list< boost::shared_ptr<data::decode::Decoder> >& decoder = stack->stack();
        BOOST_FOREACH(boost::shared_ptr<data::decode::Decoder> dec, decoder) {
            QJsonArray ch_array;
            const srd_decoder *const d = dec->decoder();;
            const bool have_probes = (d->channels || d->opt_channels) != 0;
            if (have_probes) {
                for(std::map<const srd_channel*, int>::const_iterator i = dec->channels().begin();
                    i != dec->channels().end(); i++) {
                    QJsonObject ch_obj;
                    ch_obj[(*i).first->id] = QJsonValue::fromVariant((*i).second);
                    ch_array.push_back(ch_obj);
                }
            }

            QJsonObject options_obj;
            boost::shared_ptr<prop::binding::DecoderOptions> dec_binding(
                new prop::binding::DecoderOptions(stack, dec));
            for (GSList *l = d->options; l; l = l->next)
            {
                const srd_decoder_option *const opt =
                    (srd_decoder_option*)l->data;

                const std::map<string, GVariant*>& options = dec->options();
                std::map<string, GVariant*>::const_iterator iter = options.find(opt->id);
                if (opt->values) {
                    for (GSList *vl = opt->values; vl; vl = vl->next) {
                        GVariant *const var = (GVariant*)vl->data;
                        assert(var);
                        if (iter == options.end()) {
                            options_obj[opt->id] = QJsonValue::fromVariant(dec_binding->print_gvariant(opt->def));
                            break;
                        } else if (g_variant_compare((*iter).second, var) == 0) {
                            options_obj[opt->id] = QJsonValue::fromVariant(dec_binding->print_gvariant(var));
                            break;
                        }
                    }
                } else if (g_variant_is_of_type(opt->def, G_VARIANT_TYPE("d"))) {
                    GVariant *const var = dec_binding->getter(opt->id);
                    options_obj[opt->id] = QJsonValue::fromVariant(g_variant_get_double(var));
                    g_variant_unref(var);
                } else if (g_variant_is_of_type(opt->def, G_VARIANT_TYPE("x"))) {
                    GVariant *const var = dec_binding->getter(opt->id);
                    options_obj[opt->id] = QJsonValue::fromVariant(get_double(var));
                    g_variant_unref(var);
                } else if (g_variant_is_of_type(opt->def, G_VARIANT_TYPE("s"))) {
                    GVariant *const var = dec_binding->getter(opt->id);
                    options_obj[opt->id] = QJsonValue::fromVariant(g_variant_get_string(var, NULL));
                    g_variant_unref(var);
                }else {
                    continue;
                }
            }


            if (have_probes) {
                dec_obj["id"] = QJsonValue::fromVariant(d->id);
                dec_obj["channel"] = ch_array;
                dec_obj["options"] = options_obj;
            } else {
                QJsonObject stack_obj;
                stack_obj["id"] = QJsonValue::fromVariant(d->id);
                stack_obj["options"] = options_obj;
                stack_array.push_back(stack_obj);
            }
            show_obj[d->id] = QJsonValue::fromVariant(dec->shown());
        }
        dec_obj["stacked decoders"] = stack_array;


        std::map<const pv::data::decode::Row, bool> rows = stack->get_rows_gshow();
        for (std::map<const pv::data::decode::Row, bool>::const_iterator i = rows.begin();
            i != rows.end(); i++) {
            show_obj[(*i).first.title()] = QJsonValue::fromVariant((*i).second);
        }
        dec_obj["show"] = show_obj;

        dec_array.push_back(dec_obj);
    }
    return dec_array;
}

void StoreSession::load_decoders(dock::ProtocolDock *widget, QJsonArray dec_array)
{
    if (_session.get_device()->dev_inst()->mode != LOGIC ||
        dec_array.empty())
        return;

    foreach (const QJsonValue &dec_value, dec_array) {
        QJsonObject dec_obj = dec_value.toObject();
        const vector< boost::shared_ptr<view::DecodeTrace> > pre_dsigs(
            _session.get_decode_signals());
        if (widget->sel_protocol(dec_obj["id"].toString()))
            widget->add_protocol(true);
        const vector< boost::shared_ptr<view::DecodeTrace> > aft_dsigs(
            _session.get_decode_signals());

        if (aft_dsigs.size() > pre_dsigs.size()) {
            const GSList *l;
            boost::shared_ptr<view::DecodeTrace> new_dsig = aft_dsigs.back();
            const boost::shared_ptr<data::DecoderStack>& stack = new_dsig->decoder();

            if (dec_obj.contains("stacked decoders")) {
                foreach(const QJsonValue &value, dec_obj["stacked decoders"].toArray()) {
                    QJsonObject stacked_obj = value.toObject();

                    GSList *dl = g_slist_copy((GSList*)srd_decoder_list());
                    for(; dl; dl = dl->next) {
                        const srd_decoder *const d = (srd_decoder*)dl->data;
                        assert(d);

                        if (QString::fromUtf8(d->id) == stacked_obj["id"].toString()) {
                            stack->push(boost::shared_ptr<data::decode::Decoder>(
                                new data::decode::Decoder(d)));
                            break;
                        }
                    }
                    g_slist_free(dl);
                }
            }

            const std::list< boost::shared_ptr<data::decode::Decoder> >& decoder = stack->stack();
            BOOST_FOREACH(boost::shared_ptr<data::decode::Decoder> dec, decoder) {
                const srd_decoder *const d = dec->decoder();
                QJsonObject options_obj;

                if (dec == decoder.front()) {
                    std::map<const srd_channel*, int> probe_map;
                    // Load the mandatory channels
                    for(l = d->channels; l; l = l->next) {
                        const struct srd_channel *const pdch =
                            (struct srd_channel *)l->data;
                        foreach (const QJsonValue &value, dec_obj["channel"].toArray()) {
                            QJsonObject ch_obj = value.toObject();
                            if (ch_obj.contains(pdch->id)) {
                                probe_map[pdch] = ch_obj[pdch->id].toInt();
                                break;
                            }
                        }
                    }

                    // Load the optional channels
                    for(l = d->opt_channels; l; l = l->next) {
                        const struct srd_channel *const pdch =
                            (struct srd_channel *)l->data;
                        foreach (const QJsonValue &value, dec_obj["channel"].toArray()) {
                            QJsonObject ch_obj = value.toObject();
                            if (ch_obj.contains(pdch->id)) {
                                probe_map[pdch] = ch_obj[pdch->id].toInt();
                                break;
                            }
                        }
                    }
                    dec->set_probes(probe_map);
                    options_obj = dec_obj["options"].toObject();
                } else {
                    foreach(const QJsonValue &value, dec_obj["stacked decoders"].toArray()) {
                        QJsonObject stacked_obj = value.toObject();
                        if (QString::fromUtf8(d->id) == stacked_obj["id"].toString()) {
                            options_obj = stacked_obj["options"].toObject();
                            break;
                        }
                    }
                }

                for (l = d->options; l; l = l->next) {
                    const srd_decoder_option *const opt =
                        (srd_decoder_option*)l->data;
                    if (options_obj.contains(opt->id)) {
                        if (opt->values) {
                            QString enum_option = options_obj[opt->id].toString();
                            for (GSList *vl = opt->values; vl; vl = vl->next) {
                                GVariant *const var = (GVariant*)vl->data;
                                assert(var);
                                if (enum_option == QString::fromUtf8(g_variant_get_string(var, NULL))) {
                                    dec->set_option(opt->id, var);
                                    break;
                                }
                            }
                        } else if (g_variant_is_of_type(opt->def, G_VARIANT_TYPE("d"))) {
                            double d_option = options_obj[opt->id].toDouble();
                            dec->set_option(opt->id, g_variant_new_double(d_option));
                        } else if (g_variant_is_of_type(opt->def, G_VARIANT_TYPE("x"))) {
                            int64_t d_option = options_obj[opt->id].toDouble();
                            GVariant *new_value = NULL;
                            const GVariantType *const type = g_variant_get_type(opt->def);
                            if (g_variant_type_equal(type, G_VARIANT_TYPE_BYTE))
                                new_value = g_variant_new_byte(d_option);
                            else if (g_variant_type_equal(type, G_VARIANT_TYPE_INT16))
                                new_value = g_variant_new_int16(d_option);
                            else if (g_variant_type_equal(type, G_VARIANT_TYPE_UINT16))
                                new_value = g_variant_new_uint16(d_option);
                            else if (g_variant_type_equal(type, G_VARIANT_TYPE_INT32))
                                new_value = g_variant_new_int32(d_option);
                            else if (g_variant_type_equal(type, G_VARIANT_TYPE_UINT32))
                                new_value = g_variant_new_int32(d_option);
                            else if (g_variant_type_equal(type, G_VARIANT_TYPE_INT64))
                                new_value = g_variant_new_int64(d_option);
                            else if (g_variant_type_equal(type, G_VARIANT_TYPE_UINT64))
                                new_value = g_variant_new_uint64(d_option);
                            if (new_value != NULL)
                                dec->set_option(opt->id, new_value);
                        } else if (g_variant_is_of_type(opt->def, G_VARIANT_TYPE("s"))) {
                            QString s_option = options_obj[opt->id].toString();
                            dec->set_option(opt->id, g_variant_new_string(s_option.toLocal8Bit().data()));
                        }
                    }
                }
                dec->commit();

                if (dec_obj.contains("show")) {
                    QJsonObject show_obj = dec_obj["show"].toObject();
                    if (show_obj.contains(d->id)) {
                        dec->show(show_obj[d->id].toBool());
                    }
                }
            }

            if (dec_obj.contains("show")) {
                QJsonObject show_obj = dec_obj["show"].toObject();
                std::map<const pv::data::decode::Row, bool> rows = stack->get_rows_gshow();
                for (std::map<const pv::data::decode::Row, bool>::const_iterator i = rows.begin();
                    i != rows.end(); i++) {
                    if (show_obj.contains((*i).first.title())) {
                        stack->set_rows_gshow((*i).first, show_obj[(*i).first.title()].toBool());
                    }
                }
            }
        }
    }

}
#endif

double StoreSession::get_double(GVariant *var)
{
    double val = 0;
    const GVariantType *const type = g_variant_get_type(var);
    assert(type);

    if (g_variant_type_equal(type, G_VARIANT_TYPE_BYTE))
        val = g_variant_get_byte(var);
    else if (g_variant_type_equal(type, G_VARIANT_TYPE_INT16))
        val = g_variant_get_int16(var);
    else if (g_variant_type_equal(type, G_VARIANT_TYPE_UINT16))
        val = g_variant_get_uint16(var);
    else if (g_variant_type_equal(type, G_VARIANT_TYPE_INT32))
        val = g_variant_get_int32(var);
    else if (g_variant_type_equal(type, G_VARIANT_TYPE_UINT32))
        val = g_variant_get_uint32(var);
    else if (g_variant_type_equal(type, G_VARIANT_TYPE_INT64))
        val = g_variant_get_int64(var);
    else if (g_variant_type_equal(type, G_VARIANT_TYPE_UINT64))
        val = g_variant_get_uint64(var);
    else
        assert(0);

    return val;
}

} // pv
