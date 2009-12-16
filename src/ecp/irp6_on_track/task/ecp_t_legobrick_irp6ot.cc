#include <stdio.h>

#include "lib/typedefs.h"
#include "lib/impconst.h"
#include "lib/com_buf.h"

#include "lib/srlib.h"
//#include "ecp_mp/task/ecp_mp_t_legobrick.h"
//#include "ecp_mp/sensor/ecp_mp_s_schunk.h"

#include "ecp/irp6_on_track/ecp_r_irp6ot.h"
//#include "ecp/common/generator/ecp_g_force.h"
//#include "ecp/common/generator/ecp_g_smooth.h"
#include "ecp/irp6_on_track/task/ecp_t_legobrick_irp6ot.h"

namespace mrrocpp {
namespace ecp {
namespace irp6ot {
namespace task {

lego_brick::lego_brick(lib::configurator &_config) : task(_config)
{
	// the robot is choose dependendat on the section of configuration file sent as argv[4]
	ecp_m_robot = new robot (*this);

	//gt = new ecp_generator_t (*this);
	//nrg = new ecp_tff_nose_run_generator (*this, 8);
	//rgg = new ecp_tff_rubik_grab_generator (*this, 8);
	//gag = new ecp_tff_gripper_approach_generator (*this, 8);
	//rfrg = new ecp_tff_rubik_face_rotate_generator (*this, 8);
	//tig = new ecp_g_teach_in (*this);
	//befg = new bias_edp_force_generator (*this);
	sg = new common::generator::smooth (*this, true);
	//wmg = new weight_meassure_generator(*this, 1);

	//go_st = new ecp_sub_task_gripper_opening(*this);

	sr_ecp_msg->message("ECP loaded");
}

void lego_brick::main_task_algorithm(void)
{
	/*
	for(;;)
	{
		sr_ecp_msg->message("Waiting for MP order");

		get_next_state ();

		sr_ecp_msg->message("Order received");

		switch ( (RCSC_ECP_STATES) mp_command.ecp_next_state.mp_2_ecp_next_state)
		{
		case ECP_WEIGHT_MEASURE_GENERATOR:
			wmg->Move();
			break;
		case ECP_GEN_TRANSPARENT:
			gt->Move();
			break;
		case ECP_GEN_BIAS_EDP_FORCE:
			befg->Move();
			break;
		case ECP_GEN_TFF_NOSE_RUN:
			nrg->Move();
			break;
		case ECP_GEN_TFF_RUBIK_GRAB:
			switch ( (RCSC_RUBIK_GRAB_PHASES) mp_command.ecp_next_state.mp_2_ecp_next_state_variant)
			{
			case RCSC_RG_FROM_OPEARTOR_PHASE_1:
				rgg->configure(0.057, 0.00005, 0);
				break;
			case RCSC_RG_FROM_OPEARTOR_PHASE_2:
				rgg->configure(0.057, 0.00005, 50);
				break;
			case RCSC_RG_FCHANGE_PHASE_1:
				rgg->configure(0.072, 0.00005, 0, false);
				break;
			case RCSC_RG_FCHANGE_PHASE_2:
				rgg->configure(0.062, 0.00005, 0);
				break;
			case RCSC_RG_FCHANGE_PHASE_3:
				rgg->configure(0.057, 0.00005, 0);
				break;
			case RCSC_RG_FCHANGE_PHASE_4:
				rgg->configure(0.057, 0.00005, 50);
				break;
			default:
				break;
			}
			rgg->Move();
			break;
		case ECP_GEN_TFF_GRIPPER_APPROACH:
			gag->configure(0.01 , 150);
			gag->Move();
			break;
		case ECP_GEN_TFF_RUBIK_FACE_ROTATE:
			switch ( (RCSC_TURN_ANGLES) mp_command.ecp_next_state.mp_2_ecp_next_state_variant)
			{
			case RCSC_CCL_90:
				rfrg->configure(-90.0);
				break;
			case RCSC_CL_0:
				rfrg->configure(0.0);
				break;
			case RCSC_CL_90:
				rfrg->configure(90.0);
				break;
			case RCSC_CL_180:
				rfrg->configure(180.0);
				break;
			default:
				break;
			}
			rfrg->Move();
			break;
		case RCSC_GRIPPER_OPENING:
			switch ( (RCSC_TURN_ANGLES) mp_command.ecp_next_state.mp_2_ecp_next_state_variant)
			{
			case RCSC_GO_VAR_1:
				go_st->configure(0.002, 1000);
				go_st->execute();
				break;
			case RCSC_GO_VAR_2:
				go_st->configure(0.02, 1000);
				go_st->execute();
				break;
			default:
				break;
			}
			break;
		case ECP_GEN_TEACH_IN:
			size = 1 + strlen(mrrocpp_network_path) + strlen(mp_command.ecp_next_state.mp_2_ecp_next_state_string);
			path1 = new char[size];
			// Stworzenie sciezki do pliku.
			strcpy(path1, mrrocpp_network_path);
			sprintf(path1, "%s%s", mrrocpp_network_path, mp_command.ecp_next_state.mp_2_ecp_next_state_string);
			tig->flush_pose_list();
			tig->load_file_with_path (path1);
			//		printf("\nTRACK ECP_GEN_TEACH_IN :%s\n\n", path1);
			tig->initiate_pose_list();
			delete[] path1;
			tig->Move();
			break;
		case ECP_GEN_SMOOTH:
			size = 1 + strlen(mrrocpp_network_path) + strlen(mp_command.ecp_next_state.mp_2_ecp_next_state_string);
			path1 = new char[size];
			// Stworzenie sciezki do pliku.
			strcpy(path1, mrrocpp_network_path);
			sprintf(path1, "%s%s", mrrocpp_network_path, mp_command.ecp_next_state.mp_2_ecp_next_state_string);
			sg->load_file_with_path (path1);
			//				printf("\nTRACK ECP_GEN_SMOOTH :%s\n\n", path1);
			delete[] path1;
			sg->Move();
			break;
		default:
			break;
		}

		ecp_termination_notice();

	} //end for
	 */

}

}
} // namespace irp6ot

namespace common {
namespace task {

task* return_created_ecp_task(lib::configurator &_config)
{
	return new irp6ot::task::lego_brick(_config);
}

}
} // namespace common
} // namespace ecp
} // namespace mrrocpp
