#ifndef WGT_SMB_COMMAND_H
#define WGT_SMB_COMMAND_H

#include <QtGui/QWidget>
#include <QVBoxLayout>
#include <QDockWidget>
#include "ui_wgt_smb_command.h"
#include "../base/wgt_base.h"
#include <QTimer>

#include <boost/shared_ptr.hpp>

namespace mrrocpp {
namespace ui {
namespace common {
class Interface;
class UiRobot;
}
namespace smb {
class UiRobot;
const std::string WGT_SMB_COMMAND = "WGT_SMB_COMMAND";
}
}
}

class wgt_smb_command : public wgt_base
{
	Q_OBJECT

public:
	wgt_smb_command(const QString & _widget_label, mrrocpp::ui::common::Interface& _interface, mrrocpp::ui::common::UiRobot *_robot, QWidget *parent =
			0);

	QVector <QCheckBox*> checkBox_fl_in_Vector;
	QVector <QCheckBox*> checkBox_fl_out_Vector;
	QVector <QCheckBox*> checkBox_fl_attached_Vector;
	QVector <QCheckBox*> checkBox_fl_undetachable_Vector;

	QVector <QCheckBox*> checkBox_m_mip_Vector;
	QVector <QCheckBox*> checkBox_m_no_Vector;

	QVector <QRadioButton*> radioButton_fl_in_Vector;
	QVector <QRadioButton*> radioButton_fl_out_Vector;

	QVector <QDoubleSpinBox*> doubleSpinBox_m_current_position_Vector;
	QVector <QDoubleSpinBox*> doubleSpinBox_m_absolute_Vector;
	QVector <QDoubleSpinBox*> doubleSpinBox_m_relative_Vector;

	void synchro_depended_init();

	void showEvent(QShowEvent * event);

private:
	Ui::wgt_smb_commandClass ui;
	mrrocpp::ui::smb::UiRobot* robot;

	void init();

	void synchro_depended_widgets_disable(bool _set_disabled);

	void get_desired_position();
	void move_it();

	boost::shared_ptr <QTimer> timer;

	signals:
	void synchro_depended_init_signal();

private slots:

	void synchro_depended_init_slot();

	void timer_slot();

	// buttons callbacks

	void on_pushButton_fl_execute_clicked();
	void on_pushButton_fl_all_in_clicked();
	void on_pushButton_fl_all_out_clicked();
	void on_pushButton_m_execute_clicked();
	void on_pushButton_stop_clicked();

	void on_pushButton_read_clicked();
	void on_pushButton_ml_copy_clicked();
	void on_pushButton_ms_copy_clicked();
	void on_pushButton_ml_left_clicked();
	void on_pushButton_ml_rigth_clicked();
	void on_pushButton_ms_left_clicked();
	void on_pushButton_ms_rigth_clicked();

	void on_radioButton_m_motor_toggled();
	void on_radioButton_m_joint_toggled();
	void on_radioButton_m_ext_toggled();

};

#endif // WGT_SMB_COMMAND_H
