#include <cstdio>

#include "base/lib/typedefs.h"
#include "base/lib/impconst.h"
#include "base/lib/com_buf.h"

#include "base/lib/sr/srlib.h"
#include "ecp_mp_t_rcsc.h"
#include "subtask/ecp_st_smooth_file_from_mp.h"
#include "generator/ecp/tff_gripper_approach/ecp_mp_g_tff_gripper_approach.h"
#include "generator/ecp/tff_rubik_face_rotate/ecp_mp_g_tff_rubik_face_rotate.h"

#include "ecp_t_rcsc.h"
#include "generator/ecp/bias_edp_force/ecp_g_bias_edp_force.h"
#include "generator/ecp/tff_nose_run/ecp_g_tff_nose_run.h"
#include "generator/ecp/sleep/ecp_g_sleep.h"
#include "generator/ecp/transparent/ecp_mp_g_transparent.h"
#include "generator/ecp/ecp_mp_g_newsmooth.h"
#include "generator/ecp/ecp_mp_g_teach_in.h"
#include "generator/ecp/weight_measure/ecp_mp_g_weight_measure.h"

namespace mrrocpp {
namespace ecp {
namespace common {
namespace task {

rcsc::rcsc(lib::configurator &_config) :
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

	register_generator(new common::generator::sleep(*this));
	register_generator(new generator::transparent(*this));
	register_generator(new generator::bias_edp_force(*this));
	register_generator(new generator::tff_gripper_approach(*this, 8));
	register_generator(new generator::tff_rubik_face_rotate(*this, 8));
	{
		common::generator::tff_nose_run *ecp_gen = new common::generator::tff_nose_run(*this, 8);
		register_generator(ecp_gen);
	}

	register_generator(new generator::weight_measure(*this, 1));

	register_subtask(new sub_task::sub_task_smooth_file_from_mp(*this, lib::ECP_JOINT, ecp_mp::sub_task::ECP_ST_SMOOTH_JOINT_FILE_FROM_MP, true));
	register_subtask(new sub_task::sub_task_smooth_file_from_mp(*this, lib::ECP_XYZ_ANGLE_AXIS, ecp_mp::sub_task::ECP_ST_SMOOTH_ANGLE_AXIS_FILE_FROM_MP, true));

	sr_ecp_msg->message("ecp loaded");
}

rcsc::~rcsc()
{

}

}
} // namespace irp6ot

namespace common {
namespace task {

task_base* return_created_ecp_task(lib::configurator &_config)
{
	return new common::task::rcsc(_config);
}

}
} // namespace common
} // namespace ecp
} // namespace mrrocpp

