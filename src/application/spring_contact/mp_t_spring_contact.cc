/*!
 * @file
 * @brief File contains mp_task class definition
 * @author twiniars <twiniars@ia.pw.edu.pl>, Warsaw University of Technology
 *
 * @ingroup spring_contact
 */

#include <iostream>
#include <sstream>

#include "base/mp/mp_task.h"

#include "mp_t_spring_contact.h"
#include "base/lib/mrmath/mrmath.h"

#include "robot/irp6_tfg/dp_tfg.h"

#include "ecp_mp_g_spring_contact.h"
#include "generator/ecp/force/ecp_mp_g_bias_edp_force.h"
#include "generator/ecp/force/ecp_mp_g_tff_nose_run.h"
#include "generator/ecp/ecp_mp_g_tfg.h"

#include "robot/irp6ot_m/mp_r_irp6ot_m.h"
#include "robot/irp6p_m/mp_r_irp6p_m.h"
#include "robot/irp6ot_tfg/mp_r_irp6ot_tfg.h"
#include "robot/irp6p_tfg/mp_r_irp6p_tfg.h"

namespace mrrocpp {
namespace mp {
namespace task {

task* return_created_mp_task(lib::configurator &_config)
{
	return new spring_contact(_config);
}

spring_contact::spring_contact(lib::configurator &_config) :
		task(_config)
{
}

// powolanie robotow w zaleznosci od zawartosci pliku konfiguracyjnego
void spring_contact::create_robots()
{
	ACTIVATE_MP_ROBOT(irp6p_m);
}

void spring_contact::main_task_algorithm(void)
{

	sr_ecp_msg->message("New spring_contact series");

	// sekwencja generator na wybranym manipulatorze
	set_next_ecp_state(ecp_mp::generator::ECP_GEN_BIAS_EDP_FORCE, (int) 5, "", lib::irp6p_m::ROBOT_NAME);

	wait_for_task_termination(false, 1, lib::irp6p_m::ROBOT_NAME.c_str());

	for (;;) {

		set_next_ecp_state(ecp_mp::generator::ECP_GEN_TFF_NOSE_RUN, (int) 0, "", lib::irp6p_m::ROBOT_NAME);

		wait_for_task_termination(false, 1, lib::irp6p_m::ROBOT_NAME.c_str());

		set_next_ecp_state(ecp_mp::generator::SPRING_CONTACT, (int) 0, "", lib::irp6p_m::ROBOT_NAME);

		wait_for_task_termination(false, 1, lib::irp6p_m::ROBOT_NAME.c_str());
	}

	sr_ecp_msg->message("END");

}

} // namespace task
} // namespace mp
} // namespace mrrocpp
