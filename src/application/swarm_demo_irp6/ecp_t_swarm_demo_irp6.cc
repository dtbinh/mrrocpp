#include "base/lib/configurator.h"
#include "base/lib/sr/sr_ecp.h"

#include "base/ecp/ecp_task.h"
#include "ecp_t_swarm_demo_irp6.h"
#include "subtask/ecp_st_smooth_file_from_mp.h"
#include "generator/ecp/force/ecp_mp_g_bias_edp_force.h"
#include "generator/ecp/force/ecp_g_bias_edp_force.h"
#include "generator/ecp/force/ecp_mp_g_tff_gripper_approach.h"

#include "robot/irp6p_m/const_irp6p_m.h"

namespace mrrocpp {
namespace ecp {
namespace common {
namespace task {

// KONSTRUKTORY
swarm_demo::swarm_demo(lib::configurator &_config) :
		common::task::task(_config)
{
	// the robot is choose dependently on the section of configuration file sent as argv[4]
	if (config.robot_name == lib::irp6ot_m::ROBOT_NAME) {
		ecp_m_robot = (boost::shared_ptr <robot_t>) new irp6ot_m::robot(*this);
	} else if (config.robot_name == lib::irp6p_m::ROBOT_NAME) {
		ecp_m_robot = (boost::shared_ptr <robot_t>) new irp6p_m::robot(*this);
	} else {
		// TODO: throw
		throw std::runtime_error("Robot not supported");
	}

	// utworzenie generatorow
	gag = new common::generator::tff_gripper_approach(*this, 8);

	// utworzenie generatorow do uruchamiania dispatcherem
	generator_m[ecp_mp::generator::ECP_GEN_BIAS_EDP_FORCE] = new generator::bias_edp_force(*this);

	// utworzenie podzadan
	subtask_m[ecp_mp::sub_task::ECP_ST_SMOOTH_JOINT_FILE_FROM_MP] =
			new sub_task::sub_task_smooth_file_from_mp(*this, lib::ECP_JOINT, true);
	subtask_m[ecp_mp::sub_task::ECP_ST_SMOOTH_ANGLE_AXIS_FILE_FROM_MP] =
			new sub_task::sub_task_smooth_file_from_mp(*this, lib::ECP_XYZ_ANGLE_AXIS, true);

	sr_ecp_msg->message("ecp SWARM DEMO irp6 loaded");
}

void swarm_demo::mp_2_ecp_next_state_string_handler(void)
{
	if (mp_2_ecp_next_state_string == ecp_mp::generator::ECP_GEN_TFF_GRIPPER_APPROACH) {
		gag->configure(0.02, 300, 3);
		gag->Move();
	}
}

task_base* return_created_ecp_task(lib::configurator &_config)
{
	return new common::task::swarm_demo(_config);
}

}
} // namespace common
} // namespace ecp
} // namespace mrrocpp
