#include <cstdio>

#include "base/lib/typedefs.h"
#include "base/lib/impconst.h"
#include "base/lib/com_buf.h"

#include "base/lib/sr/srlib.h"

#include "robot/smb/ecp_r_smb1.h"
#include "robot/smb/ecp_r_smb2.h"
#include "generator/ecp/ecp_g_newsmooth.h"
#include "generator/ecp/sleep/ecp_g_sleep.h"
#include "ecp_g_smb.h"
#include "ecp_t_smb.h"
#include "generator/ecp/transparent/ecp_mp_g_transparent.h"
#include "generator/ecp/ecp_mp_g_newsmooth.h"
#include "generator/ecp/sleep/ecp_mp_g_sleep.h"
#include "ecp_mp_g_smb.h"

namespace mrrocpp {
namespace ecp {
namespace smb {
namespace task {

swarmitfix::swarmitfix(lib::configurator &_config) :
		common::task::_task <ecp::smb::robot>(_config)
{
	// Robot is choosen dependending on the section of configuration file sent as argv[4].
	if (config.robot_name == lib::smb1::ROBOT_NAME) {
		ecp_m_robot = (boost::shared_ptr <robot_t>) new smb1::robot(*this);
	} else if (config.robot_name == lib::smb2::ROBOT_NAME) {
		ecp_m_robot = (boost::shared_ptr <robot_t>) new smb2::robot(*this);
	} else {
		// TODO: throw?
	}

//	gt = new common::generator::transparent(*this);

	g_sleep = new common::generator::sleep(*this);

	g_legs_command = new smb::generator::legs_command(*this);

	g_external_epos_command = new smb::generator::external_epos_command(*this);

	sr_ecp_msg->message("ecp smb transparent loaded");
}

void swarmitfix::mp_2_ecp_next_state_string_handler(void)
{

	if (mp_2_ecp_next_state_string == ecp_mp::generator::ECP_GEN_TRANSPARENT) {
		gt->throw_kinematics_exceptions = (bool) mp_command.ecp_next_state.variant;
		gt->Move();

	} else if (mp_2_ecp_next_state_string == ecp_mp::generator::ECP_GEN_SLEEP) {
		g_sleep->init_time(mp_command.ecp_next_state.variant);
		g_sleep->Move();
	} else if (mp_2_ecp_next_state_string == ecp_mp::smb::generator::ECP_LEGS_COMMAND) {
		g_legs_command->Move();
	} else if (mp_2_ecp_next_state_string == ecp_mp::smb::generator::ECP_EXTERNAL_EPOS_COMMAND) {
		g_external_epos_command->Move();
	}

}

}
} // namespace smb

namespace common {
namespace task {

task_base* return_created_ecp_task(lib::configurator &_config)
{
	return new smb::task::swarmitfix(_config);
}

}
} // namespace common
} // namespace ecp
} // namespace mrrocpp
