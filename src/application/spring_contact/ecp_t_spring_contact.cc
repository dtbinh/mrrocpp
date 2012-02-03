/*!
 * @file
 * @brief File contains ecp_task class definition
 * @author twiniars <twiniars@ia.pw.edu.pl>, Warsaw University of Technology
 *
 * @ingroup spring_contact
 */

#include <cstdio>

#include "base/lib/configurator.h"
#include "base/lib/sr/sr_ecp.h"

#include "robot/irp6ot_m/ecp_r_irp6ot_m.h"
#include "robot/irp6p_m/ecp_r_irp6p_m.h"

#include "robot/irp6p_m/const_irp6p_m.h"

#include "ecp_t_spring_contact.h"

#include "ecp_st_spring_contact.h"
#include "generator/ecp/force/ecp_g_bias_edp_force.h"
#include "generator/ecp/force/ecp_g_tff_nose_run.h"

namespace mrrocpp {
namespace ecp {
namespace common {
namespace task {

// KONSTRUKTORY
spring_contact::spring_contact(lib::configurator &_config) :
		common::task::task(_config)
{
	// the robot is choose depending on the section of configuration file sent as argv[4]
	if (config.robot_name == lib::irp6ot_m::ROBOT_NAME) {
		ecp_m_robot = (boost::shared_ptr <robot_t>) new irp6ot_m::robot(*this);
	} else if (config.robot_name == lib::irp6p_m::ROBOT_NAME) {
		ecp_m_robot = (boost::shared_ptr <robot_t>) new irp6p_m::robot(*this);
	} else {
		// TODO: throw
	}

	// utworzenie generatorow do uruchamiania dispatcherem
	generator_m[ecp_mp::generator::ECP_GEN_BIAS_EDP_FORCE] = new generator::bias_edp_force(*this);

	{
		generator::tff_nose_run *ecp_gen = new generator::tff_nose_run(*this, 8);
		ecp_gen->configure_pulse_check(true);
		ecp_gen->configure_behaviour(lib::CONTACT, lib::CONTACT, lib::CONTACT, lib::UNGUARDED_MOTION, lib::UNGUARDED_MOTION, lib::UNGUARDED_MOTION);
		generator_m[ecp_mp::generator::ECP_GEN_TFF_NOSE_RUN] = ecp_gen;
	}

	// utworzenie podzadan
	{
		sub_task::sub_task* ecpst;
		ecpst = new sub_task::spring_contact(*this);
		subtask_m[ecp_mp::sub_task::SPRING_CONTACT] = ecpst;
	}

	sr_ecp_msg->message("ecp spring_contact loaded");
}

task_base* return_created_ecp_task(lib::configurator &_config)
{
	return new common::task::spring_contact(_config);
}

}
} // namespace common
} // namespace ecp
} // namespace mrrocpp
