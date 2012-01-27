#ifndef WGT_SPKM_EXT_H
#define WGT_SPKM_EXT_H

#include <QtGui/QWidget>
#include <QVBoxLayout>
#include <QDockWidget>
#include "ui_wgt_spkm_ext.h"
#include "../base/wgt_base.h"
#include <QTimer>
#include "robot/spkm/dp_spkm.h"
#include <boost/shared_ptr.hpp>

namespace mrrocpp {
namespace ui {
namespace common {
class Interface;
class UiRobot;
}
namespace spkm {
class UiRobot;
const std::string WGT_SPKM_EXT = "WGT_SPKM_EXT";
}
}
}

class wgt_spkm_ext : public wgt_base
{
	Q_OBJECT

public:
	wgt_spkm_ext(const QString & _widget_label, mrrocpp::ui::common::Interface& _interface, mrrocpp::ui::common::UiRobot *_robot, QWidget *parent =
			0);

	QVector <QDoubleSpinBox*> doubleSpinBox_cur_Vector;
	//QVector <QDoubleSpinBox*> doubleSpinBox_mcur_Vector;
	QVector <QDoubleSpinBox*> doubleSpinBox_des_Vector;
	QVector <QRadioButton*> radioButton_mip_Vector;

	mrrocpp::lib::spkm::POSE_SPECIFICATION current_pose_specification;

private:
	Ui::wgt_spkm_extClass ui;
	mrrocpp::ui::spkm::UiRobot* robot;

	void init();
	void copy();

	void set_single_axis(int axis,
	//	QDoubleSpinBox* qdsb_mcur,
	QAbstractButton* qab_mip);
	void get_desired_position();
	void move_it();
	boost::shared_ptr <QTimer> timer;
	void showEvent(QShowEvent * event);

private slots:
	void timer_slot();
	void on_pushButton_read_clicked();
	void on_pushButton_export_clicked();
	void on_pushButton_exportxml_clicked();

	void on_pushButton_import_clicked();
	void on_pushButton_importxml_clicked();
	void on_pushButton_copy_clicked();
	void on_pushButton_stop_clicked();
	void on_pushButton_brake_clicked();

	void on_pushButton_execute_clicked();
	void on_pushButton_0l_clicked();
	void on_pushButton_1l_clicked();
	void on_pushButton_2l_clicked();
	void on_pushButton_3l_clicked();
	void on_pushButton_4l_clicked();
	void on_pushButton_5l_clicked();
	void on_pushButton_0r_clicked();
	void on_pushButton_1r_clicked();
	void on_pushButton_2r_clicked();
	void on_pushButton_3r_clicked();
	void on_pushButton_4r_clicked();
	void on_pushButton_5r_clicked();

	void on_radioButton_no_tool_toggled();
	void on_radioButton_tool_oriented_toggled();

};

#endif // WGT_SPKM_EXT_H
