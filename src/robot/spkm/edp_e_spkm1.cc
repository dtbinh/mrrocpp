#include "edp_e_spkm1.h"
#include "const_spkm1.h"
#include "kinematic_model_spkm.h"
#include "kinematic_parameters_spkm1.h"

namespace mrrocpp {
namespace edp {
namespace spkm1 {

#include "base/lib/debug.hpp"

effector::effector(common::shell &_shell) :
	spkm::effector(_shell, lib::spkm1::ROBOT_NAME)
{
	DEBUG_METHOD;

	if (!robot_test_mode) {
		// Create EPOS objects according to CAN ID-mapping.
		axisA = (boost::shared_ptr <maxon::epos>) new maxon::epos(*gateway, 5, "A");
		axisB = (boost::shared_ptr <maxon::epos>) new maxon::epos(*gateway, 4, "B");
		axisC = (boost::shared_ptr <maxon::epos>) new maxon::epos(*gateway, 6, "C");
		axis1 = (boost::shared_ptr <maxon::epos>) new maxon::epos(*gateway, 3, "1");
		axis2 = (boost::shared_ptr <maxon::epos>) new maxon::epos(*gateway, 2, "2");
		axis3 = (boost::shared_ptr <maxon::epos>) new maxon::epos(*gateway, 1, "3");

		// Collect axes into common array container.
		axes[0] = axisA;
		axes[1] = axisB;
		axes[2] = axisC;
		axes[3] = axis1;
		axes[4] = axis2;
		axes[5] = axis3;

		// Setup the axis array for the IPM handler
		{
			boost::unique_lock <boost::mutex> lock(ipm_handler.mtx);
			ipm_handler.axes = this->axes;
		}
	}

	// Create SPKM kinematics.
	create_kinematic_models_for_given_robot();
}

void effector::create_kinematic_models_for_given_robot(void)
{
	DEBUG_METHOD;

	// Add SPKM kinematics.
	add_kinematic_model(new kinematics::spkm::kinematic_model_spkm(kinematics::spkm1::kinematic_parameters_spkm1()));
	// Set active model
	set_kinematic_model(0);
}

} // namespace spkm1


namespace common {

// Create spkm effector.
effector* return_created_efector(common::shell &_shell)
{
	return new spkm1::effector(_shell);
}

} // namespace common
} // namespace edp
} // namespace mrrocpp
