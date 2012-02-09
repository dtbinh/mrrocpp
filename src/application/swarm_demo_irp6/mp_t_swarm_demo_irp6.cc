#include <iostream>
#include <string>
#include <sstream>

#include "base/mp/mp_task.h"
#include "mp_t_swarm_demo_irp6.h"

#include "base/lib/mrmath/mrmath.h"

#include "robot/irp6_tfg/dp_tfg.h"
#include "robot/irp6ot_tfg/const_irp6ot_tfg.h"
#include "robot/irp6p_tfg/const_irp6p_tfg.h"
#include "robot/irp6ot_m/const_irp6ot_m.h"
#include "robot/irp6p_m/const_irp6p_m.h"

#include "base/lib/typedefs.h"
#include "base/lib/impconst.h"
#include "base/lib/com_buf.h"
#include "base/lib/sr/srlib.h"

#include "generator/ecp/tff_gripper_approach/ecp_mp_g_tff_gripper_approach.h"

#include "subtask/ecp_mp_st_smooth_file_from_mp.h"
#include "generator/ecp/bias_edp_force/ecp_mp_g_bias_edp_force.h"

#include "robot/irp6ot_m/mp_r_irp6ot_m.h"
#include "robot/irp6p_m/mp_r_irp6p_m.h"

namespace mrrocpp {
namespace mp {
namespace task {

task* return_created_mp_task(lib::configurator &_config)
{
	return new swarm_demo(_config);
}

// powolanie robotow w zaleznosci od zawartosci pliku konfiguracyjnego
void swarm_demo::create_robots()
{
	ACTIVATE_MP_ROBOT(irp6ot_m);
	ACTIVATE_MP_ROBOT(irp6p_m);
}

swarm_demo::swarm_demo(lib::configurator &_config) :
		task(_config)
{
}

void swarm_demo::main_task_algorithm(void)
{
	sr_ecp_msg->message("Swarm Demo irp6 (MP) START");

	sr_ecp_msg->message("Both Joint");

	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_JOINT_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_track_joint.trj", lib::irp6ot_m::ROBOT_NAME);
	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_JOINT_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_postument_joint.trj", lib::irp6p_m::ROBOT_NAME);
	wait_for_task_termination(false, 2, lib::irp6ot_m::ROBOT_NAME.c_str(), lib::irp6p_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Both Bias");

	set_next_ecp_state(ecp_mp::generator::ECP_GEN_BIAS_EDP_FORCE, 5, "", lib::irp6ot_m::ROBOT_NAME);
	set_next_ecp_state(ecp_mp::generator::ECP_GEN_BIAS_EDP_FORCE, 5, "", lib::irp6p_m::ROBOT_NAME);
	wait_for_task_termination(false, 2, lib::irp6ot_m::ROBOT_NAME.c_str(), lib::irp6p_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Postument Angle axis");

	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_ANGLE_AXIS_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_postument_angle_p1.trj", lib::irp6p_m::ROBOT_NAME);
	wait_for_task_termination(false, 1, lib::irp6p_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Postument Force approach");
	set_next_ecp_state(ecp_mp::generator::ECP_GEN_TFF_GRIPPER_APPROACH, (int) ecp_mp::generator::tff_gripper_approach::behaviour_specification, ecp_mp::generator::tff_gripper_approach::behaviour_specification_data_type(0.02, 300, 3), lib::irp6p_m::ROBOT_NAME);

	wait_for_task_termination(false, 1, lib::irp6p_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Track Joint");

	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_JOINT_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_track_joint_p1.trj", lib::irp6ot_m::ROBOT_NAME);
	wait_for_task_termination(false, 1, lib::irp6ot_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Track angle axis");

	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_ANGLE_AXIS_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_track_angle_p1a.trj", lib::irp6ot_m::ROBOT_NAME);
	wait_for_task_termination(false, 1, lib::irp6ot_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Track Force approach");

	set_next_ecp_state(ecp_mp::generator::ECP_GEN_TFF_GRIPPER_APPROACH, (int) ecp_mp::generator::tff_gripper_approach::behaviour_specification, ecp_mp::generator::tff_gripper_approach::behaviour_specification_data_type(0.02, 300, 3), lib::irp6ot_m::ROBOT_NAME);

	wait_for_task_termination(false, 1, lib::irp6ot_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Postument angle axis2");

	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_ANGLE_AXIS_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_postument_angle_p2.trj", lib::irp6p_m::ROBOT_NAME);
	wait_for_task_termination(false, 1, lib::irp6p_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Postument Force approach");

	set_next_ecp_state(ecp_mp::generator::ECP_GEN_TFF_GRIPPER_APPROACH, (int) ecp_mp::generator::tff_gripper_approach::behaviour_specification, ecp_mp::generator::tff_gripper_approach::behaviour_specification_data_type(0.02, 300, 3), lib::irp6p_m::ROBOT_NAME);

	wait_for_task_termination(false, 1, lib::irp6p_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Track angle axis2");

	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_ANGLE_AXIS_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_track_angle_p2.trj", lib::irp6ot_m::ROBOT_NAME);
	wait_for_task_termination(false, 1, lib::irp6ot_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Track Force approach");
	set_next_ecp_state(ecp_mp::generator::ECP_GEN_TFF_GRIPPER_APPROACH, (int) ecp_mp::generator::tff_gripper_approach::behaviour_specification, ecp_mp::generator::tff_gripper_approach::behaviour_specification_data_type(0.02, 300, 3), lib::irp6ot_m::ROBOT_NAME);

	wait_for_task_termination(false, 1, lib::irp6ot_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Wait");

	wait_ms(2000);

	sr_ecp_msg->message("Both angle axis");

	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_ANGLE_AXIS_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_track_angle.trj", lib::irp6ot_m::ROBOT_NAME);
	set_next_ecp_state(ecp_mp::sub_task::ECP_ST_SMOOTH_ANGLE_AXIS_FILE_FROM_MP, 5, "../../src/application/swarm_demo_irp6/trajectory_postument_angle.trj", lib::irp6p_m::ROBOT_NAME);
	wait_for_task_termination(false, 2, lib::irp6ot_m::ROBOT_NAME.c_str(), lib::irp6p_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("Swarm Demo END");

}

} // namespace task
} // namespace mp
} // namespace mrrocpp
