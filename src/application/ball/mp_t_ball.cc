// -------------------------------------------------------------------------
//                              task/mp_t_ball.cc
//
// MP task for two robot ball device
//
// -------------------------------------------------------------------------

#include <boost/assign/list_of.hpp>

#include "base/lib/typedefs.h"
#include "base/lib/impconst.h"
#include "base/lib/com_buf.h"

#include "base/lib/sr/srlib.h"

#include "base/mp/mp_task.h"
#include "application/ball/mp_g_ball.h"
#include "application/ball/mp_t_ball.h"
#include "application/rcsc/ecp_mp_t_rcsc.h"
#include "robot/irp6ot_m/const_irp6ot_m.h"
#include "robot/irp6p_m/const_irp6p_m.h"
#include "generator/ecp/force/ecp_mp_g_bias_edp_force.h"
#include "generator/ecp/ecp_mp_g_transparent.h"
#include "generator/ecp/ecp_mp_g_newsmooth.h"

#include "robot/conveyor/mp_r_conveyor.h"
#include "robot/irp6ot_m/mp_r_irp6ot_m.h"
#include "robot/irp6p_m/mp_r_irp6p_m.h"

#include "robot/bird_hand/mp_r_bird_hand.h"
#include "robot/irp6ot_tfg/mp_r_irp6ot_tfg.h"
#include "robot/irp6p_tfg/mp_r_irp6p_tfg.h"
#include "robot/shead/mp_r_shead.h"
#include "robot/spkm/mp_r_spkm.h"
#include "robot/smb/mp_r_smb.h"
#include "robot/sarkofag/mp_r_sarkofag.h"
#include "robot/festival/const_festival.h"

namespace mrrocpp {
namespace mp {
namespace task {

task* return_created_mp_task(lib::configurator &_config)
{
	return new ball(_config);
}

// powolanie robotow w zaleznosci od zawartosci pliku konfiguracyjnego
void ball::create_robots()
{

	ACTIVATE_MP_ROBOT(irp6ot_tfg);
	ACTIVATE_MP_ROBOT(irp6ot_m);
	ACTIVATE_MP_ROBOT(irp6p_tfg);
	ACTIVATE_MP_ROBOT(irp6p_m);

}

ball::ball(lib::configurator &_config) :
		task(_config)
{
}

void ball::configure_edp_force_sensor(bool configure_track, bool configure_postument)
{
	if (configure_track) {

		set_next_ecp_state(ecp_mp::generator::ECP_GEN_BIAS_EDP_FORCE, 0, "", lib::irp6ot_m::ROBOT_NAME);
	}

	if (configure_postument) {
		set_next_ecp_state(ecp_mp::generator::ECP_GEN_BIAS_EDP_FORCE, 0, "", lib::irp6p_m::ROBOT_NAME);
	}

	if ((configure_track) && (!configure_postument)) {
		wait_for_task_termination(false, 1, lib::irp6ot_m::ROBOT_NAME.c_str());
	} else if ((!configure_track) && (configure_postument)) {
		wait_for_task_termination(false, 1, lib::irp6p_m::ROBOT_NAME.c_str());
	} else if ((configure_track) && (configure_postument)) {
		wait_for_task_termination(false, 2, lib::irp6ot_m::ROBOT_NAME.c_str(), lib::irp6p_m::ROBOT_NAME.c_str());
	}
}

void ball::main_task_algorithm(void)
{
	generator::ball mp_h_gen(*this, 10);
	mp_h_gen.robot_m = robot_m;

	//TODO do zamiany na newsmooth
	//set_next_ecp_state(ecp_mp::generator::ECP_GEN_SMOOTH, (int) ecp_mp::task::ABSOLUTE, "src/application/ball/irp6ot_init.trj", 0, lib::irp6ot_m::ROBOT_NAME);
	//set_next_ecp_state(ecp_mp::generator::ECP_GEN_SMOOTH, (int) ecp_mp::task::ABSOLUTE, "src/application/ball/irp6p_init.trj", 0, lib::irp6p_m::ROBOT_NAME);

	wait_for_task_termination(false, 2, lib::irp6ot_m::ROBOT_NAME.c_str(), lib::irp6p_m::ROBOT_NAME.c_str());

	sr_ecp_msg->message("New series");
	// wlaczenie generatora do konfiguracji czujnika w EDP w obydwu robotach
	configure_edp_force_sensor(true, true);

	// wlaczenie generatora transparentnego w obu robotach
	set_next_ecp_state(ecp_mp::generator::ECP_GEN_TRANSPARENT, 0, "", lib::irp6ot_m::ROBOT_NAME);
	set_next_ecp_state(ecp_mp::generator::ECP_GEN_TRANSPARENT, 0, "", lib::irp6p_m::ROBOT_NAME);

	mp_h_gen.configure(1, 0);
	sr_ecp_msg->message("Track podatny do czasu wcisniecia mp_trigger");
	mp_h_gen.Move();

	send_end_motion_to_ecps(2, lib::irp6ot_m::ROBOT_NAME.c_str(), lib::irp6p_m::ROBOT_NAME.c_str());
}

} // namespace task
} // namespace mp
} // namespace mrrocpp
