#include <fstream>
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/thread/thread_time.hpp>

#include "mp_t_swarmitfix.h"

#include "planner.h"

#include "base/lib/mrmath/homog_matrix.h"
#include "base/mp/mp_exceptions.h"

#include "robot/sbench/const_sbench.h"
#include "robot/spkm/const_spkm1.h"
#include "robot/spkm/const_spkm2.h"
#include "robot/smb/const_smb1.h"
#include "robot/smb/const_smb2.h"
#include "robot/shead/const_shead1.h"
#include "robot/shead/const_shead2.h"

#include "base/lib/swarmtypes.h"
#include "base/mp/mp_robot.h"
#include "base/ecp_mp/ecp_ui_msg.h"

namespace mrrocpp {
namespace mp {
namespace task {

void swarmitfix::executeCommandItem(const Plan::PkmType::ItemType & pkmCmd, IO_t & IO)
{
	// Goal pose
	lib::Homog_matrix hm;

	if(pkmCmd.pkmToWrist().present()) {
		hm = lib::Homog_matrix(pkmCmd.pkmToWrist().get());
	} else if (pkmCmd.Xyz_Euler_Zyz().present()) {

		std::cout << "Xyz_Euler_Zyz: " <<
				pkmCmd.Xyz_Euler_Zyz()->x() << " " <<
				pkmCmd.Xyz_Euler_Zyz()->y() << " " <<
				pkmCmd.Xyz_Euler_Zyz()->z() << " " <<
				pkmCmd.Xyz_Euler_Zyz()->alpha() << " " <<
				pkmCmd.Xyz_Euler_Zyz()->beta() << " " <<
				pkmCmd.Xyz_Euler_Zyz()->gamma() << std::endl;

		hm = lib::Xyz_Euler_Zyz_vector(
				pkmCmd.Xyz_Euler_Zyz()->x(),
				pkmCmd.Xyz_Euler_Zyz()->y(),
				pkmCmd.Xyz_Euler_Zyz()->z(),
				pkmCmd.Xyz_Euler_Zyz()->alpha(),
				pkmCmd.Xyz_Euler_Zyz()->beta(),
				pkmCmd.Xyz_Euler_Zyz()->gamma()
				);
	}

	// Setup command for the PKM
	lib::spkm::next_state_t cmd_spkm(lib::spkm::command_variant::GOAL_POSE);

	cmd_spkm.segment = lib::spkm::segment_t(hm);

	// Setup command for the HEAD
	lib::shead::next_state cmd_shead;

	switch(pkmCmd.ind() % 100) {
		case 0:
		case 20:
		case 40:
		case 60:
		case 80:
			cmd_shead.command = lib::shead::next_state::ROTATE;
			cmd_shead.pose = pkmCmd.beta7();
			break;
		default:
			throw std::runtime_error("Unexpected 'ind' in PKM plan item");
			break;
	}

	// Display PKM's command
	std::cerr << "MP: spkm" << (unsigned int) pkmCmd.agent() << " @" << pkmCmd.ind() << std::endl;
	std::cerr << "\tpose\n" << cmd_spkm.segment.goal_pose << std::endl;
	std::cerr << "\tmotion type " << cmd_spkm.segment.motion_type << std::endl;
	std::cerr << "\tduration " << cmd_spkm.segment.duration << std::endl;
	std::cerr << "\tguarded_motion " << cmd_spkm.segment.guarded_motion << std::endl;

	// Find output buffers
	IO_t::transmitters_t::spkm_t::_outputs::command_t spkm_command_buffer;
	IO_t::transmitters_t::shead_t::_outputs::command_t shead_command_buffer;
	lib::robot_name_t spkm_name, shead_name;

	switch((int) pkmCmd.agent()) {
		case 1:
			spkm_command_buffer = IO.transmitters.spkm1.outputs.command;
			shead_command_buffer = IO.transmitters.shead1.outputs.command;
			spkm_name = lib::spkm1::ROBOT_NAME;
			shead_name = lib::shead1::ROBOT_NAME;
			break;
		case 2:
			spkm_command_buffer = IO.transmitters.spkm2.outputs.command;
			shead_command_buffer = IO.transmitters.shead2.outputs.command;
			spkm_name = lib::spkm2::ROBOT_NAME;
			shead_name = lib::shead2::ROBOT_NAME;
			break;
		default:
			throw std::runtime_error("Unexpected 'agent' in PKM plan item");
			break;
	}

	// Send command if the output buffers are active
	if(spkm_command_buffer.get()) {
		spkm_command_buffer->Send(cmd_spkm);
		current_workers_status[spkm_name] = WorkersStatus::BUSY;
	}

	if(shead_command_buffer.get()) {
		shead_command_buffer->Send(cmd_shead);
		current_workers_status[shead_name] = WorkersStatus::BUSY;
	}

}

void swarmitfix::executeCommandItem(const Plan::MbaseType::ItemType & smbCmd, IO_t IO)
{
	// Setup command for the mobile base
	lib::smb::next_state_t cmd_smb(lib::smb::ACTION_LIST);

	std::cerr << "MP: smb" << (unsigned int) smbCmd.agent() << " @" << smbCmd.ind() << " # of actions = " << smbCmd.actions().item().size() << std::endl;

	// Iterate over action sequence
	BOOST_FOREACH(const Plan::MbaseType::ItemType::ActionsType::ItemType & it, smbCmd.actions().item())
	{
		std::cerr << "pin " << it.pin() << std::endl;
		std::cerr << "dPkmTheta " << it.dPkmTheta() << std::endl;

		// Setup single action
		lib::smb::action act;

		if(it.pin()) {
			act.setRotationPin(it.pin());
			act.setdThetaInd(it.dThetaInd());
		}
		// FIXME: this should be not needed anymore
//		double normalizedPkmTheta = smbCmd.pkmTheta();
//		while(normalizedPkmTheta > M_PI) normalizedPkmTheta -= 2*M_PI;
//		while(normalizedPkmTheta < -M_PI) normalizedPkmTheta += 2*M_PI;
//		act.setdPkmTheta(normalizedPkmTheta);

		// Append action to the command sequence
		cmd_smb.actions.push_back(act);
	}

	// Setup command for the bench
	lib::sbench::cbuffer cmd_sbench;

	// Find output buffers
	IO_t::transmitters_t::smb_t::_outputs::command_t smb_command_buffer;
	lib::robot_name_t smb_name;

	switch((int) smbCmd.agent()) {
		case 1:
			smb_command_buffer = IO.transmitters.smb1.outputs.command;
			smb_name = lib::smb1::ROBOT_NAME;
			break;
		case 2:
			smb_command_buffer = IO.transmitters.smb2.outputs.command;
			smb_name = lib::smb2::ROBOT_NAME;
			break;
		default:
			throw std::runtime_error("Unexpected 'agent' in MBASE plan item");
			break;
	}

	// Send commands if the output buffers are active
	if(smb_command_buffer.get()) {
		smb_command_buffer->Send(cmd_smb);
		current_workers_status[smb_name] = WorkersStatus::BUSY;
	}
	if(IO.transmitters.sbench.outputs.command.get()) {
		IO.transmitters.sbench.outputs.command->Send(cmd_sbench);
		current_workers_status[lib::sbench::ROBOT_NAME] = WorkersStatus::BUSY;
	}
}


void swarmitfix::handleNotification(const lib::robot_name_t & robot_name, IO_t::transmitters_t::inputs_t & inputs)
{
	if(inputs.notification.get() && inputs.notification->isFresh()) {
		inputs.notification->markAsUsed();
		current_workers_status[robot_name] = WorkersStatus::IDLE;

		if(inputs.notification->Get() == lib::NACK) {
			current_plan_status = FAILURE;
		}
	}
}

void swarmitfix::save_plan(const Plan & p)
{
	std::cout << "Save to file." << std::endl;
	std::string savepath = config.value<std::string>(planner::planpath);
	std::ofstream ofs ("foo.xml"/*savepath.c_str()*/);
	plan(ofs, p);
}

lib::UI_TO_ECP_REPLY swarmitfix::step_mode(Mbase::ItemType & item)
{
	// Create the text representation.
	std::ostringstream ostr;
	{
		boost::archive::text_oarchive oa(ostr);
		xml_schema::ostream<boost::archive::text_oarchive> os(oa);

		// serialize data
		os << item;
	}

	// Request
	lib::ECP_message ecp_to_ui_msg;

	// Setup plan item
	ecp_to_ui_msg.ecp_message = lib::PLAN_STEP_MODE;
	ecp_to_ui_msg.plan_item_type = lib::MBASE_AND_BENCH;
	ecp_to_ui_msg.plan_item_string = ostr.str();

	// Reply
	lib::UI_reply ui_to_ecp_rep;

	if (messip::port_send(UI_fd, 0, 0, ecp_to_ui_msg, ui_to_ecp_rep) < 0) {

		uint64_t e = errno;
		perror("ecp operator_reaction(): Send() to UI failed");
		sr_ecp_msg->message(lib::SYSTEM_ERROR, e, "ecp: Send() to UI failed");
		BOOST_THROW_EXCEPTION(exception::se());
	}

	if(ui_to_ecp_rep.reply == lib::PLAN_EXEC) {
		std::istringstream istr(ui_to_ecp_rep.plan_item_string);
		boost::archive::text_iarchive ia(istr);
		xml_schema::istream<boost::archive::text_iarchive> is (ia);

		// deserialize data
		item = Mbase::ItemType(is);
	}

	return ui_to_ecp_rep.reply;
}

lib::UI_TO_ECP_REPLY swarmitfix::step_mode(Pkm::ItemType & item)
{
	// create archive
	std::ostringstream ostr;
	{
		boost::archive::text_oarchive oa(ostr);
		xml_schema::ostream<boost::archive::text_oarchive> os(oa);

		// serialize data
		os << item;
	}

	// Request
	lib::ECP_message ecp_to_ui_msg;

	// Setup plan item
	ecp_to_ui_msg.ecp_message = lib::PLAN_STEP_MODE;
	ecp_to_ui_msg.plan_item_type = lib::PKM_AND_HEAD;
	ecp_to_ui_msg.plan_item_string = ostr.str();

	// Reply
	lib::UI_reply ui_to_ecp_rep;

	if (messip::port_send(UI_fd, 0, 0, ecp_to_ui_msg, ui_to_ecp_rep) < 0) {

		uint64_t e = errno;
		perror("ecp operator_reaction(): Send() to UI failed");
		sr_ecp_msg->message(lib::SYSTEM_ERROR, e, "ecp: Send() to UI failed");
		BOOST_THROW_EXCEPTION(exception::se());
	}

	if(ui_to_ecp_rep.reply == lib::PLAN_EXEC) {
		std::istringstream istr(ui_to_ecp_rep.plan_item_string);
		boost::archive::text_iarchive ia(istr);
		xml_schema::istream<boost::archive::text_iarchive> is (ia);

		// deserialize data
		item = Pkm::ItemType(is);
	}

	return ui_to_ecp_rep.reply;
}

void swarmitfix::main_test_algorithm(void)
{
	sr_ecp_msg->message("swarm test started");

	// operator_reaction("Does it work?");

	// Create planner object
	// planner pp(config.value<std::string>("planpath"));

	sr_ecp_msg->message("plan OK");

	Plan * p = pp.getPlan();

	// Time index counter
	int indMin = 0, indMax = 0;

	// Setup index counter at the beginning of the plan
	BOOST_FOREACH(const Plan::PkmType::ItemType & it, p->pkm().item()) {
		if(indMin > it.ind()) indMin = it.ind();
		if(indMax < it.ind()) indMax = it.ind();
	}
	BOOST_FOREACH(const Plan::MbaseType::ItemType & it, p->mbase().item()) {
		if(indMin > it.ind()) indMin = it.ind();
		if(indMax < it.ind()) indMax = it.ind();
	}

	current_plan_status = ONGOING;

	for (int ind = indMin, dir = 0; current_plan_status == ONGOING; ind += dir) {

		if(ind < indMin) ind = indMin;
		if(ind > indMax) ind = indMax;

		std::cout << "plan index = " << ind << std::endl;

		// Diagnostic timestamp
		boost::system_time start_timestamp;

		// Plan iterators
		const Plan::PkmType::ItemSequence::iterator pkm_it = StateAtInd(ind, p->pkm().item());
		const Plan::MbaseType::ItemSequence::iterator smb_it = StateAtInd(ind, p->mbase().item());

		// Current state
		State * currentActionState;

		// Execute matching command item
		if(pkm_it != p->pkm().item().end()) {

			switch(step_mode(*pkm_it)) {
				case lib::PLAN_PREV:
					dir = -1;
					break;
				case lib::PLAN_NEXT:
					dir = +1;
					break;
				case lib::PLAN_EXEC:
					currentActionState = (State *) &(*pkm_it);
					start_timestamp = boost::get_system_time();
					executeCommandItem(*pkm_it, IO);
					dir = 0;
					break;
				case lib::PLAN_SAVE:
					save_plan(*p);
					dir = 0;
					break;
				default:
					break;
			}
		} else if(smb_it != p->mbase().item().end()) {

			switch(step_mode(*smb_it)) {
				case lib::PLAN_PREV:
					dir = -1;
					break;
				case lib::PLAN_NEXT:
					dir = +1;
					break;
				case lib::PLAN_EXEC:
					currentActionState = (State *) &(*smb_it);
					start_timestamp = boost::get_system_time();
					executeCommandItem(*smb_it, IO);
					dir = 0;
					break;
				case lib::PLAN_SAVE:
					save_plan(*p);
					dir = 0;
					break;
				default:
					break;
			}
		} else {
			continue;
		}

		const bool record_timestamp = !current_workers_status.allIdle();

		while(!current_workers_status.allIdle()) {
			std::cout << "MP blocking for message" << std::endl;
			ReceiveSingleMessage(true);

			// Discard communication on control data channel
			BOOST_FOREACH(const common::robot_pair_t & robot_node, robot_m)
			{
				if (robot_node.second->reply.isFresh()) {

					robot_node.second->reply.markAsUsed();

					sr_ecp_msg->message("Unexpected communication on control data channel");
				}
			}

			handleNotification(lib::sbench::ROBOT_NAME, IO.transmitters.sbench.inputs);
			handleNotification(lib::shead1::ROBOT_NAME, IO.transmitters.shead1.inputs);
			handleNotification(lib::shead2::ROBOT_NAME, IO.transmitters.shead2.inputs);
			handleNotification(lib::spkm1::ROBOT_NAME, IO.transmitters.spkm1.inputs);
			handleNotification(lib::spkm2::ROBOT_NAME, IO.transmitters.spkm2.inputs);
			handleNotification(lib::smb1::ROBOT_NAME, IO.transmitters.smb1.inputs);
			handleNotification(lib::smb2::ROBOT_NAME, IO.transmitters.smb2.inputs);
		}

		// Diagnostic timestamp
		if (record_timestamp) {
			// Get the stop timestamp
			boost::system_time stop_timestamp = boost::get_system_time();

			// Calculate command duration
			boost::posix_time::time_duration td = stop_timestamp - start_timestamp;

			std::cout << "Command duration in [ms] is " << td.total_milliseconds() << std::endl;
			currentActionState->state_reached_in_time().set(td.total_milliseconds()/1000.0);

//			// Wait for trigger
//			while(!(ui_pulse.isFresh() && ui_pulse.Get() == MP_TRIGGER)) {
//				// TODO: handle PAUSE/RESUME/STOP commands as well
//				if(ui_pulse.isFresh()) ui_pulse.markAsUsed();
//				ReceiveSingleMessage(true);
//			}
//
//			ui_pulse.markAsUsed();

		}

		// If all iterators are at the end...
		if(ind == indMax && dir > 0)
		{
			// ...then finish
			break;
		}
	}

	// Serialize to a file.
	//
	{
		std::cout << "Serialize to a file." << std::endl;
		std::ofstream ofs ("result.xml");
		plan(ofs, *p);
	}

	sr_ecp_msg->message("END");
}

} // namespace task
} // namespace mp
} // namespace mrrocpp
