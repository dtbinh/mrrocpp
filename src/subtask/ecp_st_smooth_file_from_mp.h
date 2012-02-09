#if !defined(_ECP_SUB_TASK_SMOOTH_FILE_FROM_MP_H)
#define _ECP_SUB_TASK_SMOOTH_FILE_FROM_MP_H

#include <boost/shared_ptr.hpp>

#include "base/ecp/ecp_sub_task.h"
#include "ecp_mp_st_smooth_file_from_mp.h"

namespace mrrocpp {
namespace ecp {
namespace common {

namespace generator {
class newsmooth;
}

namespace sub_task {

class sub_task_smooth_file_from_mp : public sub_task
{
private:
	boost::shared_ptr <generator::newsmooth> sgen;
	std::string path;
	const bool detect_jerks;

public:
	sub_task_smooth_file_from_mp(task::task & _ecp_t, lib::ECP_POSE_SPECIFICATION pose_spec, std::string _LABEL, bool _detect_jerks = true);

	void conditional_execution();
};

} // namespace task
} // namespace common
} // namespace ecp
} // namespace mrrocpp

#endif
