#include "lib/impconst.h"
#include "lib/com_buf.h"

#include "lib/mis_fun.h"
#include "lib/srlib.h"
#include "robot/shead/mp_r_shead.h"

namespace mrrocpp {
namespace mp {
namespace robot {

shead::shead(task::task &mp_object_l) :
	motor_driven(lib::ROBOT_SHEAD, ECP_SHEAD_SECTION, mp_object_l, SHEAD_NUM_OF_SERVOS)
{
}

} // namespace robot
} // namespace mp
} // namespace mrrocpp
