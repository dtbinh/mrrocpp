#include <QtXml/QDomDocument>

#include "ui_ecp_r_spkm.h"
#include "ui_r_spkm.h"
#include "robot/spkm/const_spkm.h"

#include "wgt_spkm_ext.h"
#include "../base/interface.h"
#include "../base/mainwindow.h"
#include "../base/ui_robot.h"

wgt_spkm_ext::wgt_spkm_ext(const QString & _widget_label, mrrocpp::ui::common::Interface& _interface, mrrocpp::ui::common::UiRobot *_robot, QWidget *parent) :
		wgt_base(_widget_label, _interface, parent), current_pose_specification(lib::spkm::POSE_SPECIFICATION::WRIST_XYZ_EULER_ZYZ)
{
	ui.setupUi(this);
	robot = dynamic_cast <mrrocpp::ui::spkm::UiRobot *>(_robot);

	doubleSpinBox_cur_Vector.append(ui.doubleSpinBox_cur_p0);
	doubleSpinBox_cur_Vector.append(ui.doubleSpinBox_cur_p1);
	doubleSpinBox_cur_Vector.append(ui.doubleSpinBox_cur_p2);
	doubleSpinBox_cur_Vector.append(ui.doubleSpinBox_cur_p3);
	doubleSpinBox_cur_Vector.append(ui.doubleSpinBox_cur_p4);
	doubleSpinBox_cur_Vector.append(ui.doubleSpinBox_cur_p5);

	doubleSpinBox_des_Vector.append(ui.doubleSpinBox_des_p0);
	doubleSpinBox_des_Vector.append(ui.doubleSpinBox_des_p1);
	doubleSpinBox_des_Vector.append(ui.doubleSpinBox_des_p2);
	doubleSpinBox_des_Vector.append(ui.doubleSpinBox_des_p3);
	doubleSpinBox_des_Vector.append(ui.doubleSpinBox_des_p4);
	doubleSpinBox_des_Vector.append(ui.doubleSpinBox_des_p5);

	radioButton_mip_Vector.append(ui.radioButton_mip_0);
	radioButton_mip_Vector.append(ui.radioButton_mip_1);
	radioButton_mip_Vector.append(ui.radioButton_mip_2);
	radioButton_mip_Vector.append(ui.radioButton_mip_3);
	radioButton_mip_Vector.append(ui.radioButton_mip_4);
	radioButton_mip_Vector.append(ui.radioButton_mip_5);

	timer = (boost::shared_ptr <QTimer>) new QTimer(this);
	connect(timer.get(), SIGNAL(timeout()), this, SLOT(timer_slot()));
	timer->start(interface.position_refresh_interval);
	ui.radioButton_sync_trapezoidal->setChecked(true);
	ui.radioButton_no_tool->setChecked(true);
}

void wgt_spkm_ext::timer_slot()
{
	if ((dwgt->isVisible()) && (ui.checkBox_cyclic_read->isChecked())) {
		init();
	}

}

wgt_spkm_ext::~wgt_spkm_ext()
{

}

// slots

void wgt_spkm_ext::on_pushButton_read_clicked()
{
	init();
}

void wgt_spkm_ext::init()
{

	try {

		if (robot->state.edp.pid != -1) {
			if (robot->state.edp.is_synchronised) // Czy robot jest zsynchronizowany?
			{
				//ui.pushButton_execute->setDisabled(false);
				robot->ui_ecp_robot->the_robot->epos_external_reply_data_request_port.set_data =
						current_pose_specification;
				robot->ui_ecp_robot->the_robot->epos_external_reply_data_request_port.set_request();
				robot->ui_ecp_robot->execute_motion();
				robot->ui_ecp_robot->the_robot->epos_external_reply_data_request_port.get();

				for (int i = 0; i < 6; i++) {
					set_single_axis(i,
					// doubleSpinBox_mcur_Vector[i],
					radioButton_mip_Vector[i]);
				}

				lib::spkm::spkm_ext_epos_reply &ser =
						robot->ui_ecp_robot->the_robot->epos_external_reply_data_request_port.data;

				/*lib::Homog_matrix tmp_frame(ser.current_frame);
				 lib::Xyz_Angle_Axis_vector tmp_vector;
				 double current_position[6];
				 tmp_frame.get_xyz_angle_axis(tmp_vector);
				 tmp_vector.to_table(current_position);

				 for (int i = 0; i < 6; i++) {
				 doubleSpinBox_cur_Vector[i]->setValue(current_position[i]);
				 robot->desired_pos[i] = robot->current_pos[i];
				 }*/

				for (int i = 0; i < 6; i++) {
					doubleSpinBox_cur_Vector[i]->setValue(ser.current_pose[i]);
					robot->desired_pos[i] = robot->current_pos[i];
				}

			} else {
				// Wygaszanie elementow przy niezsynchronizowanym robocie
				//ui.pushButton_execute->setDisabled(true);
			}
		}

	} // end try
	CATCH_SECTION_UI_PTR

}

void wgt_spkm_ext::set_single_axis(int axis, QAbstractButton* qab_mip)
{
	lib::spkm::spkm_ext_epos_reply &ser = robot->ui_ecp_robot->the_robot->epos_external_reply_data_request_port.data;
//	qdsb_mcur->setValue(er.epos_controller[axis].current);

	if (ser.epos_controller[axis].motion_in_progress) {
		qab_mip->setChecked(true);
	} else {
		qab_mip->setChecked(false);
	}
}

void wgt_spkm_ext::on_pushButton_import_clicked()
{
	double val[robot->number_of_servos];

	interface.get_main_window()->get_lineEdit_position(val, robot->number_of_servos);

	for (int i = 0; i < 6; i++) {
		doubleSpinBox_des_Vector[i]->setValue(val[i]);
	}

}

void wgt_spkm_ext::on_pushButton_importxml_clicked()
{
	try {

		/*
		 <Xyz_Euler_Zyz>
		 <x>0.0533</x>
		 <y>0</y>
		 <z>0.436</z>
		 <alpha>3.1416</alpha>
		 <beta>0.783</beta>
		 <gamma>3.1416</gamma>
		 </Xyz_Euler_Zyz>
		 */

		QString xmlText = interface.get_main_window()->get_lineEdit_qstring();

		QDomDocument doc;
		doc.setContent(xmlText);

		QDomNodeList list;
		QString singlevalue;

		list = doc.elementsByTagName("x");
		singlevalue = list.at(0).toElement().text();
		doubleSpinBox_des_Vector[0]->setValue(boost::lexical_cast <double>(singlevalue.toStdString()));

		list = doc.elementsByTagName("y");
		singlevalue = list.at(0).toElement().text();
		doubleSpinBox_des_Vector[1]->setValue(boost::lexical_cast <double>(singlevalue.toStdString()));

		list = doc.elementsByTagName("z");
		singlevalue = list.at(0).toElement().text();
		doubleSpinBox_des_Vector[2]->setValue(boost::lexical_cast <double>(singlevalue.toStdString()));

		list = doc.elementsByTagName("alpha");
		singlevalue = list.at(0).toElement().text();
		doubleSpinBox_des_Vector[3]->setValue(boost::lexical_cast <double>(singlevalue.toStdString()));

		list = doc.elementsByTagName("beta");
		singlevalue = list.at(0).toElement().text();
		doubleSpinBox_des_Vector[4]->setValue(boost::lexical_cast <double>(singlevalue.toStdString()));

		list = doc.elementsByTagName("gamma");
		singlevalue = list.at(0).toElement().text();
		doubleSpinBox_des_Vector[5]->setValue(boost::lexical_cast <double>(singlevalue.toStdString()));

	} catch (...) {
		interface.ui_msg->message(lib::NON_FATAL_ERROR, "xml position import failed");
	}
}

void wgt_spkm_ext::on_pushButton_export_clicked()
{
	std::stringstream buffer(std::stringstream::in | std::stringstream::out);

	buffer << "edp_spkm EXTERNAL POSITION\n";

	for (int i = 0; i < 6; i++) {
		buffer << " " << doubleSpinBox_des_Vector[i]->value();
	}

	interface.ui_msg->message(buffer.str());

}

void wgt_spkm_ext::on_pushButton_exportxml_clicked()
{
	std::stringstream buffer(std::stringstream::in | std::stringstream::out);

	buffer << "edp_spkm EULER POSE (XML)\n";

	buffer << "<Xyz_Euler_Zyz>\n";
	buffer << "<x>" << doubleSpinBox_des_Vector[0]->value() << "</x>";
	buffer << "<y>" << doubleSpinBox_des_Vector[1]->value() << "</y>";
	buffer << "<z>" << doubleSpinBox_des_Vector[2]->value() << "</z>\n";
	buffer << "<alpha>" << doubleSpinBox_des_Vector[3]->value() << "</alpha>";
	buffer << "<beta>" << doubleSpinBox_des_Vector[4]->value() << "</beta>";
	buffer << "<gamma>" << doubleSpinBox_des_Vector[5]->value() << "</gamma>\n";
	buffer << "</Xyz_Euler_Zyz>\n";

	interface.ui_msg->message(buffer.str());

}

void wgt_spkm_ext::on_pushButton_copy_clicked()
{
	copy();
}

void wgt_spkm_ext::on_pushButton_stop_clicked()
{
	robot->execute_stop_motor();
}

void wgt_spkm_ext::on_pushButton_brake_clicked()
{
	robot->execute_brake_motor();
}

void wgt_spkm_ext::copy()
{

	if (robot->state.edp.pid != -1) {
		if (robot->state.edp.is_synchronised) // Czy robot jest zsynchronizowany?
		{
			ui.pushButton_execute->setDisabled(false);

			for (int i = 0; i < 6; i++) {
				doubleSpinBox_des_Vector[i]->setValue(doubleSpinBox_cur_Vector[i]->value());
			}

		} else {
			// Wygaszanie elementow przy niezsynchronizowanym robocie
			ui.pushButton_execute->setDisabled(true);
		}

	}
}

void wgt_spkm_ext::on_pushButton_execute_clicked()
{
	get_desired_position();
	move_it();
}

void wgt_spkm_ext::on_pushButton_0l_clicked()
{
	get_desired_position();
	robot->desired_pos[0] -= ui.doubleSpinBox_step_p0->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_1l_clicked()
{
	get_desired_position();
	robot->desired_pos[1] -= ui.doubleSpinBox_step_p1->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_2l_clicked()
{
	get_desired_position();
	robot->desired_pos[2] -= ui.doubleSpinBox_step_p2->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_3l_clicked()
{
	get_desired_position();
	robot->desired_pos[3] -= ui.doubleSpinBox_step_p3->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_4l_clicked()
{
	get_desired_position();
	robot->desired_pos[4] -= ui.doubleSpinBox_step_p4->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_5l_clicked()
{
	get_desired_position();
	robot->desired_pos[5] -= ui.doubleSpinBox_step_p5->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_0r_clicked()
{
	get_desired_position();
	robot->desired_pos[0] += ui.doubleSpinBox_step_p0->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_1r_clicked()
{
	get_desired_position();
	robot->desired_pos[1] += ui.doubleSpinBox_step_p1->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_2r_clicked()
{
	get_desired_position();
	robot->desired_pos[2] += ui.doubleSpinBox_step_p2->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_3r_clicked()
{
	get_desired_position();
	robot->desired_pos[3] += ui.doubleSpinBox_step_p3->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_4r_clicked()
{
	get_desired_position();
	robot->desired_pos[4] += ui.doubleSpinBox_step_p4->value();
	move_it();
}

void wgt_spkm_ext::on_pushButton_5r_clicked()
{
	get_desired_position();
	robot->desired_pos[5] += ui.doubleSpinBox_step_p5->value();
	move_it();
}

void wgt_spkm_ext::get_desired_position()
{

	if (robot->state.edp.pid != -1) {

		if (robot->state.edp.is_synchronised) {

			for (int i = 0; i < 6; i++) {
				robot->desired_pos[i] = doubleSpinBox_des_Vector[i]->value();
			}
		} else {

			for (int i = 0; i < robot->number_of_servos; i++) {
				robot->desired_pos[i] = 0.0;
			}
		}
	}
}

void wgt_spkm_ext::move_it()
{

	// wychwytania ew. bledow ECP::robot
	try {

		if (robot->state.edp.pid != -1) {

			lib::epos::EPOS_MOTION_VARIANT motion_variant = lib::epos::NON_SYNC_TRAPEZOIDAL;
			lib::spkm::POSE_SPECIFICATION tool_variant = current_pose_specification;

			double estimated_time = ui.doubleSpinBox_estimated_time->value();

			if (ui.radioButton_non_sync_trapezoidal->isChecked()) {
				motion_variant = lib::epos::NON_SYNC_TRAPEZOIDAL;
			} else if (ui.radioButton_sync_trapezoidal->isChecked()) {
				motion_variant = lib::epos::SYNC_TRAPEZOIDAL;
			} else if (ui.radioButton_sync_polynomal->isChecked()) {
				motion_variant = lib::epos::SYNC_POLYNOMIAL;
			} else if (ui.radioButton_operational->isChecked()) {
				motion_variant = lib::epos::OPERATIONAL;
			}
			/*
			 if (ui.radioButton_no_tool->isChecked()) {
			 tool_variant = lib::spkm::XYZ_EULER_ZYZ;
			 } else if (ui.radioButton_tool_oriented->isChecked()) {
			 tool_variant = lib::spkm::TOOL_ORIENTED_XYZ_EULER_ZYZ_WITH_TOOL;
			 } else if (ui.radioButton_wrist_oriented->isChecked()) {
			 tool_variant = lib::spkm::WRIST_ORIENTED_XYZ_EULER_ZYZ_WITH_TOOL;
			 }
			 */
			robot->ui_ecp_robot->move_external(robot->desired_pos, motion_variant, tool_variant, estimated_time);

			if ((robot->state.edp.is_synchronised) /* TR && (is_open)*/) { // by Y o dziwo nie dziala poprawnie 	 if (robot->state.edp.is_synchronised)
				for (int i = 0; i < 6; i++) {
					doubleSpinBox_des_Vector[i]->setValue(robot->desired_pos[i]);
				}

				init();
			}
		} // end if (robot->state.edp.pid!=-1)
	} // end try

	CATCH_SECTION_UI_PTR
}

void wgt_spkm_ext::on_radioButton_no_tool_toggled()
{
	if (ui.radioButton_no_tool->isChecked()) {
		current_pose_specification = lib::spkm::POSE_SPECIFICATION::WRIST_XYZ_EULER_ZYZ;
		init();
	}
}

void wgt_spkm_ext::on_radioButton_tool_oriented_toggled()
{
	if (ui.radioButton_tool_oriented->isChecked()) {
		current_pose_specification = lib::spkm::POSE_SPECIFICATION::TOOL_XYZ_EULER_ZYZ;
		init();
	}
}

void wgt_spkm_ext::showEvent(QShowEvent * event)
{
//	emit gotFocus();

	init();
}

