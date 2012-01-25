/*! \file epos.cc

 \brief libEPOS - a library to control an EPOS; implementation

 \addtogroup libEPOS Library for low-level access to the EPOS motion controller

 \@{
 */

#include <cstdio>   /* Standard input/output definitions */
#include <iostream>
#include <cstring>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <cerrno>   /* Error number definitions */
#include <cstdlib>
#include <stdint.h>  /* int types with given size */
#include <cmath>
#include <sys/select.h>

#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/thread.hpp>

#include "robot/canopen/gateway.h"
#include "epos.h"

namespace mrrocpp {
namespace edp {
namespace maxon {

using namespace canopen;

/* ********************************************* */
/*    definitions used only internal in c   */
/* ********************************************* */

/* EPOS error codes (Communication Guide, 6.4)  */

/* CANopen defined error codes */
#define E_NOERR         0x00000000   ///< Error code: no error
#define E_ONOTEX        0x06020000   ///< Error code: object does not exist
#define E_SUBINEX       0x06090011   ///< Error code: subindex does not exist
#define E_OUTMEM        0x05040005   ///< Error code: out of memory
#define E_NOACCES       0x06010000   ///< Error code: Unsupported access to an object
#define E_WRITEONLY     0x06010001   ///< Error code: Attempt to read a write-only object
#define E_READONLY      0x06010002   ///< Error code: Attempt to write a read-only object
#define E_PARAMINCOMP   0x06040043   ///< Error code: general parameter incompatibility
#define E_INTINCOMP     0x06040047   ///< Error code: general internal incompatibility in the device
#define E_HWERR         0x06060000   ///< Error code: access failed due to an hardware error
#define E_PRAGNEX       0x06090030   ///< Error code: value range of parameter exceeded
#define E_PARHIGH       0x06090031   ///< Error code: value of parameter written is too high
#define E_PARLOW        0x06090032   ///< Error code: value of parameter written is too low
#define E_PARREL        0x06090036   ///< Error code: maximum value is less than minimum value
/* maxon specific error codes */
#define E_NMTSTATE      0x0f00ffc0   ///< Error code: wrong NMT state
#define E_RS232         0x0f00ffbf   ///< Error code: rs232 command illegal
#define E_PASSWD        0x0f00ffbe   ///< Error code: password incorrect
#define E_NSERV         0x0f00ffbc   ///< Error code: device not in service mode
#define E_NODEID        0x0f00fb9    ///< Error code: error in Node-ID
/* EPOS Statusword -- singe bits, see firmware spec 14.1.58 */
#define E_BIT15        0x8000      ///< bit code: position referenced to home position
#define E_BIT14        0x4000      ///< bit code: refresh cycle of power stage
#define E_BIT13        0x2000      ///< bit code: OpMode specific, some error
#define E_BIT12        0x1000      ///< bit code: OpMode specific
#define E_BIT11        0x0800      ///< bit code: NOT USED
#define E_BIT10        0x0400      ///< bit code: Target reached
#define E_BIT09        0x0200      ///< bit code: Remote (?)
#define E_BIT08        0x0100      ///< bit code: offset current measured (?)
#define E_BIT07        0x0080      ///< bit code: WARNING
#define E_BIT06        0x0040      ///< bit code: switch on disable
#define E_BIT05        0x0020      ///< bit code: quick stop
#define E_BIT04        0x0010      ///< bit code: voltage enabled
#define E_BIT03        0x0008      ///< bit code: FAULT
#define E_BIT02        0x0004      ///< bit code: operation enable
#define E_BIT01        0x0002      ///< bit code: switched on
#define E_BIT00        0x0001      ///< bit code: ready to switch on
// Interpolation buffer status bits
#define PVT_STATUS_UNDERFLOW_WARNING	E_BIT00	///< Warning: buffer underflow
#define PVT_STATUS_OVERFLOW_WARNING		E_BIT01	///< Warning: buffer overflow
#define PVT_STATUS_VELOCITY_WARNING		E_BIT02 ///< Warning: velocity value exceeded
#define PVT_STATUS_ACCELERATION_WARNING	E_BIT03	///< Warning: acceleration value exceeded
#define PVT_STATUS_UNDERFLOW_ERROR		E_BIT08 ///< Error: buffer underflow
#define PVT_STATUS_OVERFLOW_ERROR		E_BIT09	///< Error: buffer overflow
#define PVT_STATUS_VELOCITY_ERROR		E_BIT10	///< Error: velocity value exceeded
#define PVT_STATUS_ACCELERATION_ERROR	E_BIT11	///< Error: acceleration value exceeded
#define PVT_STATUS_BUFFER_ENABLED		E_BIT14	///< Status: buffer enabled
#define PVT_STATUS_IP_MODE_ACTIVE		E_BIT15	///< Status: interpolated-profile mode active
#define PVT_STATUS_WARNING				(E_BIT00|E_BIT01|E_BIT02|E_BIT03)	///< Status: WARNING
#define PVT_STATUS_ERROR				(E_BIT08|E_BIT09|E_BIT02|E_BIT03)	///< Status: ERROR
/************************************************************/
/*           EPOS related constants                         */
/************************************************************/

// FIXME: this value should be 60, but it has to be tested
const unsigned epos::SECONDS_PER_MINUTE = 60;

/************************************************************/
/*          high-level read functions */
/************************************************************/

epos::epos(gateway & _device, uint8_t _nodeId, const std::string & _deviceName) :
		device(_device), nodeId(_nodeId), deviceName(_deviceName)
{
	// Read the cached parameters
	OpMode = getActualOperationMode();
	PositionProfileType = getPositionProfileType();
	ProfileVelocity = getProfileVelocity();
	ProfileAcceleration = getProfileAcceleration();
	ProfileDeceleration = getProfileDeceleration();
	TargetVelocity = getTargetVelocity();
	remote = isRemoteOperationEnabled(getStatusWord());

	{
		UNSIGNED16 outputs = getDigitalOutputs();

		DigitalOutputs[0] = (outputs & (1 << 15)) ? true : false;
		DigitalOutputs[1] = (outputs & (1 << 14)) ? true : false;
		DigitalOutputs[2] = (outputs & (1 << 13)) ? true : false;
		DigitalOutputs[3] = (outputs & (1 << 12)) ? true : false;
		DigitalOutputs[4] = (outputs & (1 << 11)) ? true : false;
		DigitalOutputs[5] = (outputs & (1 << 10)) ? true : false;
		DigitalOutputs[6] = (outputs & (1 << 9)) ? true : false;
		DigitalOutputs[7] = (outputs & (1 << 8)) ? true : false;
	}
	// Create node description.
	nodeDescription = "'" + deviceName + "' (CanID " + boost::lexical_cast<std::string>((int) nodeId) + ")";

#if 0
	std::cout << "Node[" << (int) nodeId << "] {V,A,D} " <<
	ProfileVelocity << ", " <<
	ProfileAcceleration << ", " <<
	ProfileDeceleration << std::endl;

	std::cout << "Node[" << (int) nodeId << "] {Vmax,Amax,VmotorMax} " <<
	getMaxProfileVelocity() << ", " <<
	getMaxAcceleration() << ", " <<
	getMotorMaxSpeed() << std::endl;

	std::cout << "Gear[" << (int) nodeId << "] " <<
	getGearRatioNumerator() << "/" <<
	getGearRatioDenominator() << " maximal speed " <<
	getGearMaximalSpeed() << std::endl;
#endif
}

const std::string & epos::getDeviceName() const
{
	return nodeDescription;
}

/* read EPOS status word */
UNSIGNED16 epos::getStatusWord()
{
	return ReadObjectValue <UNSIGNED16>(0x6041, 0x00);
}

void epos::printErrorRegister(UNSIGNED8 reg)
{
	if (E_BIT07 & reg)
		std::cout << "\tMotion error\n";
	if (E_BIT06 & reg)
		std::cout << "\treserved (always 0)\n";
	if (E_BIT05 & reg)
		std::cout << "\tDevice profile-specific\n";
	if (E_BIT04 & reg)
		std::cout << "\tCommunication error\n";
	if (E_BIT03 & reg)
		std::cout << "\tTemperature error\n";
	if (E_BIT02 & reg)
		std::cout << "\tVoltage error\n";
	if (E_BIT01 & reg)
		std::cout << "\tCurrent error\n";
	if (E_BIT00 & reg)
		std::cout << "\tGeneric error\n";
}

const char * epos::ErrorCodeMessage(UNSIGNED32 code)
{
	switch (code)
	{
		case 0x0000:
			return "No Error";
		case 0x1000:
			return "Generic Error";
		case 0x2310:
			return "Overcurrent Error";
		case 0x3210:
			return "Overvoltage";
		case 0x3220:
			return "Undervoltage";
		case 0x4210:
			return "Overtemperature";
		case 0x5113:
			return "Supply Voltage (+5V) Too Low";
		case 0x5114:
			return "Supply Voltage Output Stage Too Low";
		case 0x6100:
			return "Internal Software Error";
		case 0x6320:
			return "Software Parameter Error";
		case 0x7320:
			return "Sensor Position Error";
		case 0x8110:
			return "CAN Overrun Error (Objects lost)";
		case 0x8111:
			return "CAN Overrun Error";
		case 0x8120:
			return "CAN Passive Mode Error";
		case 0x8130:
			return "CAN Life Guard Error";
		case 0x8150:
			return "CAN Transmit COB-ID Collision";
		case 0x81FD:
			return "CAN Bus Off";
		case 0x81FE:
			return "CAN Rx Queue Overrun";
		case 0x81FF:
			return "CAN Tx Queue Overrun";
		case 0x8210:
			return "CAN PDO Length Error";
		case 0x8611:
			return "Following Error";
		case 0xFF01:
			return "Hall Sensor Error";
		case 0xFF02:
			return "Index Processing Error";
		case 0xFF03:
			return "Encoder Resolution Error";
		case 0xFF04:
			return "Hall Sensor not found Error";
		case 0xFF06:
			return "Negative Limit Error";
		case 0xFF07:
			return "Positive Limit Error";
		case 0xFF08:
			return "Hall Angle Detection Error";
		case 0xFF09:
			return "Software Position Limit Error";
		case 0xFF0A:
			return "Position Sensor Breach";
		case 0xFF0B:
			return "System Overloaded";
		case 0xFF0C: {
			UNSIGNED16 status = getInterpolationBufferStatus();
			printInterpolationBufferStatus(status);
			return "Interpolated Position Mode Error";
		}
		case 0xFF0D:
			return "Auto Tuning Identification Error";
		default:
			return "Unknown error";
	}
}

void epos::printStatusWord(WORD s)
{
	printf("\nmeaning of EPOS statusword %#06x is:\n", s);

	printf("15: position referenced to home position: ");
	if ((s & E_BIT15) == E_BIT15
	)
		printf("true\n");
	else
		printf("false\n");

	printf("14: refresh cycle of power stage:         ");
	if ((s & E_BIT14) == E_BIT14
	)
		printf("true\n");
	else
		printf("false\n");

	printf("13: OpMode specific, some error:          ");
	if ((s & E_BIT13) == E_BIT13
	)
		printf("true\n");
	else
		printf("false\n");

	printf("12: OpMode specific:                      ");
	if ((s & E_BIT12) == E_BIT12
	)
		printf("true\n");
	else
		printf("false\n");

	printf("11: NOT USED                              ");
	if ((s & E_BIT11) == E_BIT11
	)
		printf("true\n");
	else
		printf("false\n");

	printf("10: Target reached:                       ");
	if ((s & E_BIT10) == E_BIT10
	)
		printf("true\n");
	else
		printf("false\n");

	printf("09: Remote (?)                            ");
	if ((s & E_BIT09) == E_BIT09
	)
		printf("true\n");
	else
		printf("false\n");

	printf("08: offset current measured (?)           ");
	if ((s & E_BIT08) == E_BIT08
	)
		printf("true\n");
	else
		printf("false\n");

	printf("07: WARNING                               ");
	if ((s & E_BIT07) == E_BIT07
	)
		printf("true\n");
	else
		printf("false\n");

	printf("06: switch on disable                     ");
	if ((s & E_BIT06) == E_BIT06
	)
		printf("true\n");
	else
		printf("false\n");

	printf("05: quick stop                            ");
	if ((s & E_BIT05) == E_BIT05
	)
		printf("true\n");
	else
		printf("false\n");

	printf("04: voltage enabled                       ");
	if ((s & E_BIT04) == E_BIT04
	)
		printf("true\n");
	else
		printf("false\n");

	printf("03: FAULT                                 ");
	if ((s & E_BIT03) == E_BIT03
	)
		printf("true\n");
	else
		printf("false\n");

	printf("02: operation enable                      ");
	if ((s & E_BIT02) == E_BIT02
	)
		printf("true\n");
	else
		printf("false\n");

	printf("01: switched on                           ");
	if ((s & E_BIT01) == E_BIT01
	)
		printf("true\n");
	else
		printf("false\n");

	printf("00: ready to switch on                    ");
	if ((s & E_BIT00) == E_BIT00
	)
		printf("true\n");
	else
		printf("false\n");
}

epos::actual_state_t epos::status2state(WORD w)
{
	/* state 'start' (0)
	 fedc ba98  7654 3210
	 w == x0xx xxx0  x000 0000 */
	if (!bitcmp(w, E_BIT00) && !bitcmp(w, E_BIT01) && !bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && !bitcmp(w, E_BIT04)
			&& !bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && !bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (START);

	/* state 'not ready to switch on' (1)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x000 0000 */
	if (!bitcmp(w, E_BIT00) && !bitcmp(w, E_BIT01) && !bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && !bitcmp(w, E_BIT04)
			&& !bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (NOT_READY_TO_SWITCH_ON);

	/* state 'switch on disabled' (2)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x100 0000 */
	if (!bitcmp(w, E_BIT00) && !bitcmp(w, E_BIT01) && !bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && !bitcmp(w, E_BIT04)
			&& !bitcmp(w, E_BIT05) && bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (SWITCH_ON_DISABLED);

	/* state 'ready to switch on' (3)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x010 0001 */
	if (bitcmp(w, E_BIT00) && !bitcmp(w, E_BIT01) && !bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && !bitcmp(w, E_BIT04)
			&& bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (READY_TO_SWITCH_ON);

	/* state 'switched on' (4)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x010 0011 */
	if (bitcmp(w, E_BIT00) && bitcmp(w, E_BIT01) && !bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && !bitcmp(w, E_BIT04)
			&& bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (SWITCHED_ON);

	/* state 'refresh' (5)
	 fedc ba98  7654 3210
	 w == x1xx xxx1  x010 0011 */
	if (bitcmp(w, E_BIT00) && bitcmp(w, E_BIT01) && !bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && !bitcmp(w, E_BIT04)
			&& bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && bitcmp(w, E_BIT14))
		return (REFRESH);

	/* state 'measure init' (6)
	 fedc ba98  7654 3210
	 w == x1xx xxx1  x011 0011 */
	if (bitcmp(w, E_BIT00) && bitcmp(w, E_BIT01) && !bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && bitcmp(w, E_BIT04)
			&& bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && bitcmp(w, E_BIT14))
		return (MEASURE_INIT);

	/* state 'operation enable' (7)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x011 0111 */
	if (bitcmp(w, E_BIT00) && bitcmp(w, E_BIT01) && bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && bitcmp(w, E_BIT04)
			&& bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (OPERATION_ENABLE);

	/* state 'quick stop active' (8)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x001 0111 */
	if (bitcmp(w, E_BIT00) && bitcmp(w, E_BIT01) && bitcmp(w, E_BIT02) && !bitcmp(w, E_BIT03) && bitcmp(w, E_BIT04)
			&& !bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (QUICK_STOP_ACTIVE);

	/* state 'fault reaction active (disabled)' (9)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x000 1111 */
	if (bitcmp(w, E_BIT00) && bitcmp(w, E_BIT01) && bitcmp(w, E_BIT02) && bitcmp(w, E_BIT03) && !bitcmp(w, E_BIT04)
			&& !bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (FAULT_REACTION_ACTIVE_DISABLED);

	/* state 'fault reaction active (enabled)' (10)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x001 1111 */
	if (bitcmp(w, E_BIT00) && bitcmp(w, E_BIT01) && bitcmp(w, E_BIT02) && bitcmp(w, E_BIT03) && bitcmp(w, E_BIT04)
			&& !bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (FAULT_REACTION_ACTIVE_ENABLED);

	/* state 'fault' (11)
	 fedc ba98  7654 3210
	 w == x0xx xxx1  x000 1000 */
	if (!bitcmp(w, E_BIT00) && !bitcmp(w, E_BIT01) && !bitcmp(w, E_BIT02) && bitcmp(w, E_BIT03) && !bitcmp(w, E_BIT04)
			&& !bitcmp(w, E_BIT05) && !bitcmp(w, E_BIT06) && bitcmp(w, E_BIT08) && !bitcmp(w, E_BIT14))
		return (FAULT);

	// if we get down here, statusword has a unknown value!
	fprintf(stderr, "WARNING: EPOS status word %#06x is an unknown state!\n", w);

	return (UNKNOWN);
}

/*! check EPOS state, firmware spec 8.1.1

 \return EPOS status as defined in firmware specification 8.1.1

 */
epos::actual_state_t epos::getState()
{
	WORD w = getStatusWord();

	//printEPOSstatusword(w);

	return status2state(w);
}

bool epos::isRemoteOperationEnabled(WORD status)
{
	return (status & E_BIT09);
}

void epos::setRemoteOperation(bool enable)
{
	if (remote != enable) {
		device.SendNMTService(nodeId, (enable) ? gateway::Start_Remote_Node : gateway::Stop_Remote_Node);
		remote = isRemoteOperationEnabled(getStatusWord());

		if (remote != enable) {
			BOOST_THROW_EXCEPTION(fe() << reason("Failed to change REMOTE state of the device"));
		}
	}
}

const char * epos::stateDescription(int state)
{
	switch (state)
	{
		case 0:
			return "start";
			break;
		case 1:
			return "not ready to switch on";
			break;
		case 2:
			return "switch on disabled";
			break;
		case 3:
			return "ready to switch on";
			break;
		case 4:
			return "switched on";
			break;
		case 5:
			return "refresh";
			break;
		case 6:
			return "measure init";
			break;
		case 7:
			return "operation enable";
			break;
		case 8:
			return "quick stop active";
			break;
		case 9:
			return "fault reaction active (disabled)";
			break;
		case 10:
			return "fault reaction active (enabled)";
			break;
		case 11:
			return "fault";
			break;
		default:
			return "unknown";
	}
}

/* pretty-print EPOS state */
int epos::printState()
{
	actual_state_t state = getState();

	std::cout << "EPOS node " << getDeviceName() << ": is in state: ";

	switch (state)
	{
		case START:
			std::cout << "Start\n";
			std::cout << "\tBootup\n";
			break;
		case NOT_READY_TO_SWITCH_ON:
			std::cout << "Not Ready to Switch On\n";
			std::cout << "\tCurrent offset will be measured\n";
			std::cout << "\tDrive function is disabled\n";
			break;
		case SWITCH_ON_DISABLED:
			std::cout << "Switch On Disabled\n";
			std::cout << "\tDrive initialization is complete\n";
			std::cout << "\tDrive parameters may be changed\n";
			std::cout << "\tDrive function is disabled\n";
			break;
		case READY_TO_SWITCH_ON:
			std::cout << "Ready to Switch On\n";
			std::cout << "\tDrive parameters may be changed\n";
			std::cout << "\tDrive function is disabled\n";
			break;
		case SWITCHED_ON:
			std::cout << "Switched On\n";
			std::cout << "\tDrive function is disabled\n";
			break;
		case REFRESH:
			std::cout << "Refresh\n";
			std::cout << "\tRefresh of power stage\n";
			break;
		case MEASURE_INIT:
			std::cout << "Measure Init\n";
			std::cout << "\tPower is applied to the motor\n";
			std::cout << "\tMotor resistance or commutation delay is measured\n";
			break;
		case OPERATION_ENABLE:
			std::cout << "Operation Enable\n";
			std::cout << "\tNo faults have been detected\n";
			std::cout << "\tDrive function is enabled and power is applied to the motor\n";
			break;
		case QUICK_STOP_ACTIVE:
			std::cout << "Quickstop Active\n";
			std::cout << "\tQuickstop function is being executed\n";
			std::cout << "\tDrive function is enabled and power is applied to the motor\n";
			break;
		case FAULT_REACTION_ACTIVE_DISABLED:
			std::cout << "Fault Reaction Active (disabled)\n";
			std::cout << "\tA fault has occurred in the drive\n";
			std::cout << "\tDrive function is disabled\n";
			break;
		case FAULT_REACTION_ACTIVE_ENABLED:
			std::cout << "Fault Reaction Active (enabled)\n";
			std::cout << "\tA fault has occurred in the drive\n";
			std::cout << "\tSelected fault reaction is being executed\n";
			break;
		case FAULT:
			std::cout << "FAULT\n";
			std::cout << "\tA fault has occurred in the drive\n";
			std::cout << "\tDrive parameters may be changed\n";
			std::cout << "\tDrive function is disabled\n";
			break;

		default:
			std::cout << "UNKNOWN!\n";
			return (-1);
	}
	return (0);
}

void epos::clearFault(void)
{
	// Print state.
	printState();
	// Check if node is in a FAULT state.
	if (getState() == FAULT) {
		UNSIGNED8 errNum = getNumberOfErrors();
		std::cout << "getNumberOfErrors() = " << (unsigned int) errNum << std::endl;
		// Print list of errors.
		for (UNSIGNED8 i = 1; i <= errNum; ++i) {
			UNSIGNED32 errCode = getErrorHistory(i);
			std::cout << "\t" << ErrorCodeMessage(errCode) << std::endl;
		}
		// Clear errors.
		if (errNum > 0) {
			clearNumberOfErrors();
		}

		// Reset errors.
		setState(maxon::epos::FAULT_RESET);

		// Wakeup timer
		boost::system_time wakeup = boost::get_system_time();

		// Recovery retry counter
		unsigned int retry = 0;

		// Recovery status flag
		bool recovered = false;

		// It takes some time to recovery from FAULT state
		while(retry++ < 5) {
			if(getState() == FAULT) {
				// Increment the wakeup time
				wakeup += boost::posix_time::milliseconds(5);

				// Wait for device state to change
				boost::thread::sleep(wakeup);
			} else {
				recovered = true;
				break;
			}
		}

		if(recovered) {
			std::cout << "EPOS node " << getDeviceName() <<
					": recovering in " << retry << " retries" << std::endl;
		} else {
			// We are not supposed to clear faults here
			BOOST_THROW_EXCEPTION(fe() << reason("Failed to recovery from FAULT state"));
		}
	}

	// Reset node.
	enable();
}

void epos::enable()
{
	// TODO: handle initial error conditions
	actual_state_t state = getState();

	std::cout << "EPOS node " << getDeviceName()
			<< ": resetting from state '" << stateDescription(state) << "'"
			<< std::endl;

	// FAULT
	if (state == FAULT) {
		UNSIGNED8 errReg = getErrorRegister();
		if (errReg) {
			std::cout << "printErrorRegister() = 0x" << std::hex << (int) errReg << std::endl;
			printErrorRegister(errReg);

			UNSIGNED8 errNum = getNumberOfErrors();
			std::cout << "Number of Errors = " << (unsigned int) errNum << std::endl;

			for (int i = 1; i < errNum + 1; ++i) {
				UNSIGNED32 errCode = getErrorHistory(i);
				std::cout << "Error at index " << i << " is " <<
						std::hex << (int) errCode << ": " <<
						ErrorCodeMessage(errCode) << std::endl;
			}
		}

		// We are not supposed to clear faults here
		BOOST_THROW_EXCEPTION(fe() << reason("Device is in the fault state"));
	} else if (state != OPERATION_ENABLE) {
		// Shutdown
		setState(SHUTDOWN);

		// Setup the wakeup time
		boost::system_time wakeup = boost::get_system_time();

		int retry = 5;
		do {
			state = getState();

			if (state == READY_TO_SWITCH_ON) {
				break;
			} else if (state == QUICK_STOP_ACTIVE) {
				break;
			} else if (state == FAULT) {
				BOOST_THROW_EXCEPTION(fe() << reason("Device is in the fault state"));
			} else {
				std::cout << "EPOS node " << getDeviceName() << ": transited to state '" << stateDescription(state)
						<< "' during shutdown" << std::endl;
				// Continue;
			}

			// Increment the wakeup time
			wakeup += boost::posix_time::milliseconds(5);

			// Wait for device state to change
			boost::thread::sleep(wakeup);

		} while (--retry);

		if (retry == 0) {
			BOOST_THROW_EXCEPTION(fe() << reason("Timeout shutting device down"));
		}

		// Ready-to-switch-On expected
		if (state != READY_TO_SWITCH_ON && state != QUICK_STOP_ACTIVE) {
			BOOST_THROW_EXCEPTION(fe() << reason("Ready-to-switch-On or Quick-Stop-Active expected"));
		}

		// Enable
		setState(ENABLE_OPERATION);

		// Setup the wakeup time
		wakeup = boost::get_system_time();

		// Setup retry counter
		retry = 25;
		do {
			state = getState();

			// Condition to monitor for
			bool in_operation_enable = false;

			switch (state) {
				// These are expected transition states
				case SWITCHED_ON:
				case MEASURE_INIT:
				case REFRESH:
					break;
				case OPERATION_ENABLE:
					in_operation_enable = true;
					break;
				case FAULT:
					BOOST_THROW_EXCEPTION(fe() << reason("Device is in the fault state"));
					break;
				default:
					std::cout << "EPOS node " << getDeviceName() << ": transited to state '" << stateDescription(state)
										<< "' during initialization" << std::endl;
					break;
			}

			// Exit loop if condition holds
			if(in_operation_enable)
				break;

			// Increment the wakeup time
			wakeup += boost::posix_time::milliseconds(5);

			// Wait for device state to change
			boost::thread::sleep(wakeup);

		} while (--retry);

		if (retry == 0) {
			BOOST_THROW_EXCEPTION(fe() << reason("Timeout enabling device"));
		}
	}

	// Enable+Halt
	setControlword(0x010f);

	state = getState();

	// Operation Enabled expected
	if (state != OPERATION_ENABLE) {
		BOOST_THROW_EXCEPTION(fe() << reason("Operation Enable expected"));
	}

	std::cout << "EPOS node " << getDeviceName() << ": reset OK" << std::endl;
}

/* change EPOS state according to firmware spec 8.1.3 */
void epos::setState(desired_state_t state)
{
	UNSIGNED16 cw = 0x0000;

	/* ! DO NOT READ OLD CONTROLWORD BACK, JUST SET THE BITS. It works
	 this way, but does NOT work otherways! -- mh, 07.07.06
	 */
	//cw = readControlword();
	switch (state)
	{
		case SHUTDOWN: // Shutdown: 0xxx x110
			cw &= ~E_BIT07;
			cw |= E_BIT02;
			cw |= E_BIT01;
			cw &= ~E_BIT00;
			setControlword(cw);
			break;
		case SWITCH_ON: // Switch On: 0xxx x111
			cw &= ~E_BIT07;
			cw |= E_BIT02;
			cw |= E_BIT01;
			cw |= E_BIT00;
			setControlword(cw);
			break;
		case SWITCH_ON_AND_ENABLE: // Switch On & Enable Operation: 0xxx 1111
			cw &= ~E_BIT07;
			cw |= E_BIT03;
			cw |= E_BIT02;
			cw |= E_BIT01;
			cw |= E_BIT00;
			setControlword(cw);
			break;
		case DISABLE_VOLTAGE: // Disable Voltage: 0xxx xx0x
			cw &= ~E_BIT07;
			cw &= ~E_BIT01;
			setControlword(cw);
			break;
		case QUICKSTOP: // Quickstop: 0xxx x01x
			cw &= ~E_BIT07;
			cw &= ~E_BIT02;
			cw |= E_BIT01;
			setControlword(cw);
			break;
		case DISABLE_OPERATION: // Disable Operation: 0xxx 0111
			cw &= ~E_BIT07;
			cw &= ~E_BIT03;
			cw |= E_BIT02;
			cw |= E_BIT01;
			cw |= E_BIT00;
			setControlword(cw);
			break;
		case ENABLE_OPERATION: // Enable Operation: 0xxx 1111
			cw &= ~E_BIT07;
			cw |= E_BIT03;
			cw |= E_BIT02;
			cw |= E_BIT01;
			cw |= E_BIT00;
			setControlword(cw);
			break;
		case FAULT_RESET: // Fault Reset 0xxx xxxx -> 1xxx xxxx
			cw |= E_BIT07;
			setControlword(0x0000);
			setControlword(cw);
			break;
		default:
			BOOST_THROW_EXCEPTION(fe() << reason("ERROR: demanded state is UNKNOWN!"));
			// TODO: state
			break;
	}
}

UNSIGNED8 epos::getNodeID()
{
	return ReadObjectValue <UNSIGNED8>(0x2000, 0x00);
}

/* returns software version as HEX  --  14.1.33*/
UNSIGNED16 epos::getSWversion()
{
	return ReadObjectValue <UNSIGNED16>(0x2003, 0x01);
}

/* read digital input functionality polarity -- firmware spec 14.1.47 */
UNSIGNED16 epos::getDInputPolarity()
{
	return ReadObjectValue <UNSIGNED16>(0x2071, 0x03);
}

void epos::setDInputPolarity(UNSIGNED16 val)
{
	WriteObjectValue(0x2071, 0x03, val);
}

UNSIGNED16 epos::getDInputExecMask()
{
	return ReadObjectValue <UNSIGNED16>(0x2071, 0x02);
}

void epos::setDInputExecMask(UNSIGNED16 val)
{
	WriteObjectValue(0x2071, 0x02, val);
}

/* read digital input */
UNSIGNED16 epos::getDInput()
{
	return ReadObjectValue <UNSIGNED16>(0x2071, 0x01);
}

/* set home switch polarity -- firmware spec 14.1.47 */
void epos::setHomePolarity(int pol)
{
	if (pol != 0 && pol != 1) {
		BOOST_THROW_EXCEPTION(fe() << reason("polarity must be 0 (height active) or 1 (low active)"));
	}

	// read present functionalities polarity mask
	WORD mask = getDInputPolarity();

	// set bit 2 (==home switch) to 0 or 1:
	if (pol == 0)
		mask &= ~E_BIT02;
	else if (pol == 1)
		mask |= E_BIT02;

	WriteObjectValue(0x2071, 0x03, mask);
}

/* read EPOS control word (firmware spec 14.1.57) */
UNSIGNED16 epos::getControlword()
{
	return ReadObjectValue <UNSIGNED16>(0x6040, 0x00);
}

/* write EPOS control word (firmware spec 14.1.57) */
void epos::setControlword(UNSIGNED16 val)
{
	WriteObjectValue(0x6040, 0x00, val);
}

/* pretty-print Controlword */
void epos::printControlWord(WORD s)
{
	std::cout << "\nmeaning of EPOS controlword " <<
			std::hex << "0x" << (int) s << " is:\n";
	// bit 15..11 not in use
	// bit 10, 9 reserved
	std::cout << "  HALT:                                 ";
	if ((s & E_BIT08) == E_BIT08)
		std::cout << "true\n";
	else
		std::cout << "false\n";

	std::cout << "  fault reset                           ";
	if ((s & E_BIT07) == E_BIT07)
		std::cout << "true\n";
	else
		std::cout << "false\n";

	std::cout << "  Op mode specific                      ";
	if ((s & E_BIT06) == E_BIT06)
		std::cout << "true\n";
	else
		std::cout << "false\n";

	std::cout << "  Op mode specific                      ";
	if ((s & E_BIT05) == E_BIT05)
		std::cout << "true\n";
	else
		std::cout << "false\n";

	std::cout << "  Op mode specific                      ";
	if ((s & E_BIT04) == E_BIT04)
		std::cout << "true\n";
	else
		std::cout << "false\n";

	std::cout << "  enable operation                      ";
	if ((s & E_BIT03) == E_BIT03)
		std::cout << "true\n";
	else
		std::cout << "false\n";

	std::cout << "  quick stop                            ";
	if ((s & E_BIT02) == E_BIT02)
		std::cout << "true\n";
	else
		std::cout << "false\n";

	std::cout << "  enable voltage                        ";
	if ((s & E_BIT01) == E_BIT01)
		std::cout << "true\n";
	else
		std::cout << "false\n";

	std::cout << "  switch on                             ";
	if ((s & E_BIT00) == E_BIT00)
		std::cout << "true\n";
	else
		std::cout << "false\n";
}

void epos::startAbsoluteMotion()
{
	setControlword(0x003f);
}

void epos::startRelativeMotion()
{
	setControlword(0x005f);
}

/* set mode of operation --- 14.1.59 */
void epos::setOperationMode(operational_mode_t m)
{
	if (OpMode != m) {
		WriteObjectValue<INTEGER8>(0x6060, 0x00, (int8_t) m);

		OpMode = m;
	}
}

/* read mode of operation --- 14.1.60 */
epos::operational_mode_t epos::getActualOperationMode()
{
	INTEGER8 mode = ReadObjectValue <INTEGER8>(0x6061, 0x00);
	return (operational_mode_t) mode;
}

/* read demand position; 14.1.61 */
INTEGER32 epos::getDemandPosition()
{
	return ReadObjectValue <INTEGER32>(0x6062, 0x00);
}

/* read actual position; firmware description 14.1.62 */
INTEGER32 epos::getActualPosition()
{
	return ReadObjectValue <INTEGER32>(0x6064, 0x00);
}

/* read position window; 14.1.64 */
UNSIGNED32 epos::getPositionWindow()
{
	return ReadObjectValue <UNSIGNED32>(0x6067, 0x00);
}

/* write  position window; 14.1.64 */
void epos::setPositionWindow(UNSIGNED32 val)
{
	WriteObjectValue(0x6067, 0x00, val);
}

INTEGER32 epos::getTargetVelocity()
{
	return ReadObjectValue <INTEGER32>(0x60FF, 0x00);
}

void epos::setTargetVelocity(INTEGER32 val)
{
	if(val != TargetVelocity) {
		WriteObjectValue(0x60FF, 0x00, val);
		TargetVelocity = val;
	}
}

void epos::setVelocityModeSettingValue(INTEGER32 val)
{
	WriteObjectValue(0x206B, 0x00, val);
}

void epos::setProfileVelocity(UNSIGNED32 val)
{
	if (ProfileVelocity != val) {
#if 0
		std::cout << "ProfileVelocity[" << (int) nodeId << "] <= " << val << std::endl;
#endif
		WriteObjectValue(0x6081, 0x00, val);
		ProfileVelocity = val;
#if 0
		std::cout << "ProfileVelocity[" << (int) nodeId << "] <= " << getProfileVelocity() << std::endl;
#endif
	}
}

void epos::setProfileAcceleration(UNSIGNED32 val)
{
	if (ProfileAcceleration != val) {
		WriteObjectValue(0x6083, 0x00, val);
#if 0
		std::cout << "ProfileAcceleration[" << (int) nodeId << "] <= " << val << std::endl;
#endif
		ProfileAcceleration = val;
#if 0
		std::cout << "ProfileAcceleration[" << (int) nodeId << "] <= " << getProfileAcceleration() << std::endl;
#endif
	}
}

void epos::setProfileDeceleration(UNSIGNED32 val)
{
	if (ProfileDeceleration != val) {
		WriteObjectValue(0x6084, 0x00, val);
#if 0
		std::cout << "ProfileDeceleration[" << (int) nodeId << "] <= " << val << std::endl;
#endif
		ProfileDeceleration = val;
#if 0
		std::cout << "ProfileDeceleration[" << (int) nodeId << "] <= " << getProfileDeceleration() << std::endl;
#endif
	}
}

void epos::setQuickStopDeceleration(UNSIGNED32 val)
{
	WriteObjectValue(0x6085, 0x00, val);
}

void epos::setMaxProfileVelocity(UNSIGNED32 val)
{
	WriteObjectValue(0x607F, 0x00, val);
}

void epos::setMaxAcceleration(UNSIGNED32 val)
{
	WriteObjectValue(0x60C5, 0x00, val);
}

void epos::setPositionProfileType(INTEGER16 type)
{
	if (PositionProfileType != type) {
		WriteObjectValue(0x6086, 0x00, type);
		PositionProfileType = type;
	}
}

UNSIGNED32 epos::getProfileVelocity()
{
	return ReadObjectValue <UNSIGNED32>(0x6081, 0x00);
}

UNSIGNED32 epos::getProfileAcceleration()
{
	return ReadObjectValue <UNSIGNED32>(0x6083, 0x00);
}

UNSIGNED32 epos::getProfileDeceleration()
{
	return ReadObjectValue <UNSIGNED32>(0x6084, 0x00);
}

UNSIGNED32 epos::getQuickStopDeceleration()
{
	return ReadObjectValue <UNSIGNED32>(0x6085, 0x00);
}

UNSIGNED32 epos::getMaxProfileVelocity()
{
	return ReadObjectValue <UNSIGNED32>(0x607F, 0x00);
}

UNSIGNED32 epos::getMaxAcceleration()
{
	return ReadObjectValue <UNSIGNED32>(0x60C5, 0x00);
}

INTEGER16 epos::getPositionProfileType()
{
	return ReadObjectValue <INTEGER16>(0x6086, 0x00);
}

// by Martí Morta
/* Velocity Notation index 14.1.83 */
epos::velocity_notation_t epos::getVelocityNotationIndex()
{
	return (velocity_notation_t) ReadObjectValue <INTEGER8>(0x608B, 0x00);
}

/* Velocity Notation index 14.1.83  1=0x01(1), 2=0x02(2).. 0=0x00(0), -1=0xFF(255), -2=0xFE(254) */
void epos::setVelocityNotationIndex(velocity_notation_t val)
{
	WriteObjectValue(0x608B, 0x00, val);
}

// by Martí Morta
/* read sensorConfiguration-sensor Pulses; 14.1.57 */
UNSIGNED32 epos::getSensorPulses()
{
	return ReadObjectValue <UNSIGNED32>(0x2210, 0x01);
}

/* read sensorConfiguration-sensor Type; 14.1.57 */
epos::sensor_type_t epos::getSensorType()
{
	return (sensor_type_t) ReadObjectValue <UNSIGNED16>(0x2210, 0x02);
}

/* read sensorPolarity-sensor Type; 14.1.57 */
UNSIGNED16 epos::getSensorPolarity()
{
	return ReadObjectValue <UNSIGNED16>(0x2210, 0x04);
}

/* write sensorConfiguration-sensor Pulses; 14.1.57 */
void epos::setSensorPulses(UNSIGNED32 val)
{
	WriteObjectValue(0x2210, 0x01, val);
}

/* write sensorConfiguration-sensor Type; 14.1.57 */
void epos::setSensorType(sensor_type_t val)
{
	WriteObjectValue(0x2210, 0x02, val);
}

/* write sensorPolarity-sensor Pulses; 14.1.57 */
void epos::setSensorPolarity(UNSIGNED16 val)
{
	WriteObjectValue(0x2210, 0x04, val);
}

UNSIGNED16 epos::getRS232Baudrate()
{
	return ReadObjectValue <UNSIGNED16>(0x2002, 0x00);
}

void epos::setRS232Baudrate(UNSIGNED16 val)
{
	WriteObjectValue(0x2002, 0x00, val);
}

// by Martí Morta
/* read P position; 14.1.92 */
INTEGER16 epos::getP()
{
	return ReadObjectValue <INTEGER16>(0x60FB, 0x01);
}

/* read I; 14.1.92 */
INTEGER16 epos::getI()
{
	return ReadObjectValue <INTEGER16>(0x60FB, 0x02);
}

/* read D; 14.1.92 */
INTEGER16 epos::getD()
{
	return ReadObjectValue <INTEGER16>(0x60FB, 0x03);
}

/* read Velocity Feed Forward; 14.1.92 */
UNSIGNED16 epos::getVFF()
{
	return ReadObjectValue <UNSIGNED16>(0x60FB, 0x04);
}

/* read Acceleration feed forward; 14.1.92 */
UNSIGNED16 epos::getAFF()
{
	return ReadObjectValue <UNSIGNED16>(0x60FB, 0x05);
}

/* write P; 14.1.92 */
void epos::setP(INTEGER16 val)
{
	WriteObjectValue(0x60FB, 0x01, val);
}

/* write I; 14.1.92 */
void epos::setI(INTEGER16 val)
{
	WriteObjectValue(0x60FB, 0x02, val);
}

/* write D; 14.1.92 */
void epos::setD(INTEGER16 val)
{
	WriteObjectValue(0x60FB, 0x03, val);
}

/* write VFF; 14.1.92 */
void epos::setVFF(UNSIGNED16 val)
{
	WriteObjectValue(0x60FB, 0x04, val);
}

/* write AFF; 14.1.92 */
void epos::setAFF(UNSIGNED16 val)
{
	WriteObjectValue(0x60FB, 0x05, val);
}

/* read P current; 14.1.92 */
INTEGER16 epos::getPcurrent()
{
	return ReadObjectValue <INTEGER16>(0x60F6, 0x01);
}

/* read I current; 14.1.92 */
INTEGER16 epos::getIcurrent()
{
	return ReadObjectValue <INTEGER16>(0x60F6, 0x02);
}

/* write P current; 14.1.92 */
void epos::setPcurrent(INTEGER16 val)
{
	WriteObjectValue(0x60F6, 0x01, val);
}

/* write I current; 14.1.92 */
void epos::setIcurrent(INTEGER16 val)
{
	WriteObjectValue(0x60F6, 0x02, val);
}

// by Martí Morta
/* save all parameters home; 14.1.55 */
void epos::saveParameters()
{
	// this is an alias for the original libepos API
	Store();
}

// by Martí Morta
/* write home; 14.1.55 */
INTEGER32 epos::getHomePosition()
{
	return ReadObjectValue <INTEGER32>(0x2081, 0x00);
}

void epos::setHomePosition(INTEGER32 val)
{
	WriteObjectValue(0x2081, 0x00, val);
}

// by Martí Morta
/* Motor Data 14.95 */
// Continous Current limit
UNSIGNED16 epos::setMotorContinousCurrentLimit()
{
	return ReadObjectValue <UNSIGNED16>(0x6410, 0x01);
}

void epos::getMotorContinousCurrentLimit(UNSIGNED16 cur)
{
	WriteObjectValue(0x6410, 0x01, cur);
}

// Output Current limit
UNSIGNED16 epos::getMotorOutputCurrentLimit()
{
	return ReadObjectValue <UNSIGNED16>(0x6410, 0x02);
}

void epos::setMotorOutputCurrentLimit(UNSIGNED16 cur)
{
	WriteObjectValue(0x6410, 0x02, cur);
}

// Pole Pairs -> 8 BITS
UNSIGNED8 epos::getMotorPolePairNumber()
{
	return ReadObjectValue <UNSIGNED8>(0x6410, 0x03);
}

void epos::setMotorPolePairNumber(UNSIGNED8 cur)
{
	WriteObjectValue(0x6410, 0x03, cur);
}

// Max Speed in current mode
UNSIGNED32 epos::getMotorMaxSpeed()
{
	return ReadObjectValue <UNSIGNED32>(0x6410, 0x04);
}

void epos::setMotorMaxSpeed(UNSIGNED32 val)
{
	WriteObjectValue(0x6410, 0x04, val);
}

// Thermal time constant in winding
UNSIGNED16 epos::getMotorThermalConstant()
{
	return ReadObjectValue <UNSIGNED16>(0x6410, 0x05);
}

void epos::setMotorThermalConstant(UNSIGNED16 val)
{
	WriteObjectValue(0x6410, 0x05, val);
}

//------------- fi martí

INTEGER32 epos::setDemandVelocity()
{
	return ReadObjectValue <INTEGER32>(0x606b, 0x00);
}

INTEGER32 epos::getActualVelocity()
{
	return ReadObjectValue <INTEGER32>(0x606c, 0x00);
}

INTEGER32 epos::getActualVelocityAveraged()
{
	return ReadObjectValue <INTEGER32>(0x2028, 0x00);
}

/* read actual motor current, see firmware description 14.1.69 */
INTEGER16 epos::getActualCurrent()
{
	return ReadObjectValue <INTEGER16>(0x6078, 0x00);
}

/* read EPOS target position; firmware description 14.1.70 */
INTEGER32 epos::getTargetPosition()
{
	return ReadObjectValue <INTEGER32>(0x607a, 0x00);
}

void epos::setTargetPosition(INTEGER32 val)
{
	WriteObjectValue(0x607a, 0x00, val);
}

/* read manufacturer device name string firmware */
//std::string epos::getCanDeviceName()
//{
//	WORD answer[8];
//	unsigned int r = device.ReadObject(answer, 8, nodeId, 0x1008, 0x00);
//
//	char name[16];
//
//	for (int i = 0; i < 4; ++i) {
//		name[i * 2] = (answer[3 + i] & 0xFF);
//		name[i * 2 + 1] = ((answer[3 + i] >> 8) & 0xFF);
//	}
//
//	printf("%d: %c%c%c%c%c%c%c%c\n", r, name[0], name[1], name[2], name[3], name[4], name[5], name[6], name[7]);
//
//	std::string str;
//
//	str += (char) (answer[3] & 0x00FF);
//	str += (char) ((answer[3] & 0xFF00) >> 8);
//	str += (char) (answer[4] & 0x00FF);
//	str += (char) ((answer[4] & 0xFF00) >> 8);
//	// TODO: iterate until end of string
//
//	return str;
//}

/*! read Maximal Following Error */
UNSIGNED32 epos::getMaxFollowingError()
{
	return ReadObjectValue <UNSIGNED32>(0x6065, 0x00);
}

/*! write Maximal Following Error */
void epos::setMaxFollowingError(UNSIGNED32 val)
{
	WriteObjectValue(0x6065, 0x00, val);
}

/*! read Home Offset */
INTEGER32 epos::getHomeOffset()
{
	return ReadObjectValue <INTEGER32>(0x607C, 0x00);
}

/*! write Home Offset */
void epos::setHomeOffset(INTEGER32 val)
{
	WriteObjectValue(0x607C, 0x00, val);
}

/*! read Speed for Switch Search */
UNSIGNED32 epos::getSpeedForSwitchSearch()
{
	return ReadObjectValue <UNSIGNED32>(0x6099, 0x01);
}

/*! write Speed for Switch Search */
void epos::setSpeedForSwitchSearch(UNSIGNED32 val)
{
	WriteObjectValue(0x6099, 0x01, val);
}

/*! read Speed for Zero Search */
UNSIGNED32 epos::getSpeedForZeroSearch()
{
	return ReadObjectValue <UNSIGNED32>(0x6099, 0x02);
}

/*! write Speed for Zero Search */
void epos::setSpeedForZeroSearch(UNSIGNED32 val)
{
	WriteObjectValue(0x6099, 0x02, val);
}

/*! read Homing Acceleration */
UNSIGNED32 epos::getHomingAcceleration()
{
	return ReadObjectValue <UNSIGNED32>(0x609A, 0x00);
}

/*! write Homing Acceleration  */
void epos::setHomingAcceleration(UNSIGNED32 val)
{
	WriteObjectValue(0x609A, 0x00, val);
}

/*! read Current Threshold for Homing Mode */
UNSIGNED16 epos::getCurrentThresholdForHomingMode()
{
	return ReadObjectValue <UNSIGNED16>(0x2080, 0x00);
}

/*! write Current Threshold for Homing Mode  */
void epos::setCurrentThresholdForHomingMode(UNSIGNED16 val)
{
	WriteObjectValue(0x2080, 0x00, val);
}

/*! read Homing Method */
epos::homing_method_t epos::getHomingMethod()
{
	INTEGER8 val;
	val = ReadObjectValue <INTEGER8>(0x6098, 0x00);

	return (homing_method_t) val;
}

/*! write Homing Method */
void epos::setHomingMethod(homing_method_t method)
{
	INTEGER8 val = (INTEGER8) (method);
	WriteObjectValue(0x6098, 0x00, val);
}

/*! Read the Minimal Position Limit */
INTEGER32 epos::getMinimalPositionLimit()
{
	return ReadObjectValue <INTEGER32>(0x607D, 0x01);
}

/*! Write the Minimal Position Limit */
void epos::setMinimalPositionLimit(INTEGER32 val)
{
	WriteObjectValue(0x607D, 0x01, val);
}

/*! Read the Maximal Position Limit */
INTEGER32 epos::getMaximalPositionLimit()
{
	return ReadObjectValue <INTEGER32>(0x607D, 0x02);
}

/*! Write the Maximal Position Limit */
void epos::setMaximalPositionLimit(INTEGER32 val)
{
	WriteObjectValue(0x607D, 0x02, val);
}

void epos::disablePositionLimits()
{
	this->setMinimalPositionLimit(-0x80000000);
	this->setMaximalPositionLimit(+0x7FFFFFFF);
}

UNSIGNED32 epos::getActualBufferSize()
{
	return ReadObjectValue <UNSIGNED32>(0x60C4, 0x02);
}

void epos::clearPvtBuffer()
{
	// Clear input buffer (and all data records) access disabled
	WriteObjectValue(0x60C4, 0x06, 0);

	// Enable access to the input buffer for the drive functions
	WriteObjectValue(0x60C4, 0x06, 1);
}

INTEGER16 epos::getInterpolationSubModeSelection()
{
	return ReadObjectValue <INTEGER32>(0x60C0, 0x02);
}

void epos::setInterpolationSubModeSelection(INTEGER16 val)
{
	WriteObjectValue(0x60C0, 0x00, val);
}

UNSIGNED8 epos::getInterpolationTimePeriod()
{
	return ReadObjectValue <UNSIGNED8>(0x60C2, 0x01);
}

void epos::setInterpolationTimePeriod(UNSIGNED8 val)
{
	WriteObjectValue(0x60C2, 0x01, val);
}

INTEGER8 epos::getInterpolationTimeIndex()
{
	return ReadObjectValue <INTEGER8>(0x60C2, 0x02);
}

void epos::setInterpolationTimeIndex(INTEGER8 val)
{
	WriteObjectValue(0x60C2, 0x02, val);
}

//! write Interpolation data record
void epos::setInterpolationDataRecord(INTEGER32 position, INTEGER32 velocity, UNSIGNED8 time)
{
	// only 24 bits allowed for velocity
	if (velocity > (int) 0x00ffffff || velocity < (int) 0xff000000) {
		BOOST_THROW_EXCEPTION(fe() << reason("Only 24 bits allowed for velocity"));
	}

	// This array holds a manufacturer-specific 64 bit data record
	// of a complex data structure; see Maxon documentation for details.
	uint8_t pvt[8];

	// The data layout is as follows:
	// +------------------------------------------------------------------+
	// | MSB                                                         LSB  |
	// +------------------------------------------------------------------+
	// | Time (unsigned8)  |  Velocity (signed24)  |  Position (signed32) |
	// +------------------------------------------------------------------+

	// note: velocity assignment is with 32-bit integer, so it has to
	//       be done before assignment of time value to avoid overwrite
	*((int32_t *) &pvt[4]) = (velocity & 0x00ffffff);
	pvt[7] = time;
	*((int32_t *) &pvt[0]) = position;

#if 1
	// PVT record have to be transmitted in a Segmented Write mode
	InitiateSegmentedWrite(0x20C1, 0x00, 8);
	// Maxon splits the record into two CAN frames
	SegmentedWrite(&pvt[0], 7);
	SegmentedWrite(&pvt[7], 1);
#else
	setRemoteOperation(true);
	// the cobID should be statically configured with a EPOS Studio for the TxPDO4
	WORD cobID = 0x0500 | (nodeId);
	device.SendCANFrame(cobID, 8, pvt);
#endif
}

//! read Interpolation buffer status
UNSIGNED16 epos::getInterpolationBufferStatus()
{
	return ReadObjectValue <UNSIGNED16>(0x20C4, 0x01);
}

UNSIGNED16 epos::getInterpolationBufferPosition()
{
	return ReadObjectValue <UNSIGNED16>(0x60C4, 0x04);
}

bool epos::checkInterpolationBufferWarning(UNSIGNED16 status)
{
	return (status & PVT_STATUS_WARNING);
}

bool epos::checkInterpolationBufferUnderflowWarning(UNSIGNED16 status)
{
	return (status & PVT_STATUS_UNDERFLOW_WARNING);
}

void epos::printInterpolationBufferStatus(UNSIGNED16 status)
{
	printf("IPM buffer status = 0x%04X\n", status);
	// Warning codes
	if (status & PVT_STATUS_UNDERFLOW_WARNING) {
		printf("Buffer underflow warning level is reached\n");
	}
	if (status & PVT_STATUS_OVERFLOW_WARNING) {
		printf("Buffer overflow warning level is reached\n");
	}
	if (status & PVT_STATUS_VELOCITY_WARNING) {
		printf("IPM velocity greater than profile velocity detected\n");
	}
	if (status & PVT_STATUS_ACCELERATION_WARNING) {
		printf("IPM acceleration greater than profile acceleration detected\n");
	}

	// Error codes
	if (status & PVT_STATUS_UNDERFLOW_ERROR) {
		printf("Buffer underflow error (trajectory abort)\n");
	}
	if (status & PVT_STATUS_OVERFLOW_ERROR) {
		printf("Buffer overflow error (trajectory abort)\n");
	}
	if (status & PVT_STATUS_VELOCITY_ERROR) {
		printf("IPM velocity greater than profile velocity detected\n");
	}
	if (status & PVT_STATUS_ACCELERATION_ERROR) {
		printf("IPM acceleration greater than profile acceleration detected\n");
	}

	// Status codes
	if (status & PVT_STATUS_BUFFER_ENABLED) {
		printf("Access to the input buffer enabled\n");
	}
	if (status & PVT_STATUS_IP_MODE_ACTIVE) {
		printf("IP mode active\n");
	}
}

bool epos::checkInterpolationBufferError(UNSIGNED16 status)
{
	return (status & PVT_STATUS_ERROR);
}

//! read Interpolation buffer underflow warning
UNSIGNED16 epos::getInterpolationBufferUnderflowWarning()
{
	return ReadObjectValue <UNSIGNED16>(0x20C4, 0x02);
}

//! write Interpolation buffer underflow warning
void epos::setInterpolationBufferUnderflowWarning(UNSIGNED16 val)
{
	WriteObjectValue(0x20C4, 0x02, val);
}

//! read Interpolation buffer overflow warning
UNSIGNED16 epos::getInterpolationBufferOverflowWarning()
{
	return ReadObjectValue <UNSIGNED16>(0x20C4, 0x03);
}

//! write Interpolation buffer overflow warning
void epos::setInterpolationBufferOverflowWarning(UNSIGNED16 val)
{
	WriteObjectValue(0x20C4, 0x03, val);
}

void epos::startInterpolatedPositionMotion()
{
	setControlword(0x1f);
}

/*! read Error register */
UNSIGNED8 epos::getErrorRegister()
{
	return ReadObjectValue <UNSIGNED8>(0x1001, 0x00);
}

/*! read number of Errors is Error History register */
UNSIGNED8 epos::getNumberOfErrors()
{
	return ReadObjectValue <UNSIGNED8>(0x1003, 0x00);
}

/*! read Error History at index */
UNSIGNED32 epos::getErrorHistory(unsigned int num)
{
	if (num < 1 || num > 5) {
		BOOST_THROW_EXCEPTION(fe() << reason("Error History index out of range <1..5>"));
	}
	return ReadObjectValue <UNSIGNED32>(0x1003, num);
}

/*! clear Error register */
void epos::clearNumberOfErrors()
{
	WriteObjectValue(0x1003, 0x00, (UNSIGNED8) 0x00);
}

/* firmware spec 14.1.35 */
UNSIGNED16 epos::getRS232timeout()
{
	return ReadObjectValue <UNSIGNED16>(0x2005, 0x00);
}

void epos::doSoftwareHoming(int32_t velocity_, int32_t offset_, int32_t home_position_)
{
	// Prevent from offseting in the same direction as velocity.
	assert((velocity_ > 0 && offset_ < 0) || (velocity_ < 0 && offset_ > 0));

	// Get the original limits.
	INTEGER32 originalMinPositionLimit = getMinimalPositionLimit();
	INTEGER32 originalMaxPositionLimit = getMaximalPositionLimit();

	try {
		// Disable both limits.
		disablePositionLimits();

		// Velocity mode in the direction of negative limit.
		setOperationMode(maxon::epos::OMD_VELOCITY_MODE);
		enable();

		// TODO: set max acceleration?
		setControlword(0x000f);
		setVelocityModeSettingValue(velocity_);

		// Start monitoring after some interval for acceleration.
		boost::system_time wakeup = boost::get_system_time();

		//! Actual velocity value.
		int32_t velocity;

		// Display velocity values during acceleration.
		for(int i = 0; i < 10; ++i) {
			// Increment the next wakeup time.
			wakeup += boost::posix_time::milliseconds(5);

			// Wait for device state to change.
			boost::thread::sleep(wakeup);

			velocity = getActualVelocityAveraged();

			std::cout << "software homing: acceleration phase velocity = " << velocity << std::endl;
		}

		// Startup monitoring counter.
		unsigned int monitor_counter = 0;

		do {
			// Increment the next wakeup time.
			wakeup += boost::posix_time::milliseconds(5);

			// Wait for device state to change.
			boost::thread::sleep(wakeup);

			velocity = getActualVelocityAveraged();

			if(++monitor_counter < 20) {
				// FIXME: Uncomment the following to debug the wakup/startup timer.
				 std::cout << "software homing: monitoring velocity = " << velocity << std::endl;
			}
		} while(abs(velocity) > 10);

		// Halt.
		setVelocityModeSettingValue(0);
		enable();

		try {
			// Homing: move to the index, then continue with an offset.
			setHomePosition(home_position_);

			if (offset_ > 0) {
				doHoming(maxon::epos::HM_INDEX_POSITIVE_SPEED, offset_);
			} else if (offset_ < 0) {
				doHoming(maxon::epos::HM_INDEX_NEGATIVE_SPEED, offset_);
			} else {
				doHoming(maxon::epos::HM_ACTUAL_POSITION, offset_);
			}
			// Monitor homing and set home position.
			monitorHomingStatus();

		} catch (boost::exception &e_) {
			// Motor jam!
			BOOST_THROW_EXCEPTION(fe_motor_jam_detected() << canId(nodeId));
		}

		// Revert to the original limits.
		setMinimalPositionLimit(originalMinPositionLimit);
		setMaximalPositionLimit(originalMaxPositionLimit);
	} catch (...) {
		// Revert to the original limits anyway.
		setMinimalPositionLimit(originalMinPositionLimit);
		setMaximalPositionLimit(originalMaxPositionLimit);
		// Rethrow the exception.
		throw;
	}
}

/* run the HomingMode, get the coordinate system zeropoint correct

 this is done as shown in "EPOS Application Note: device Programming,
 3: Homing Mode"

 */
int epos::doHoming(homing_method_t method, INTEGER32 offset)
{
	//move motor to a pre-defined position before the reference
	//point. This will speed-up things if the coordinates are not too
	//wrong.

	//moveAbsolute(start);

	// wait for positioning to finish, set timeout to approx. 30sec
	// CAUSES BIG PROBLEMS IF WE DO NOT WAIT!
	//waitForTarget(30);
	//monitorStatus();

	// switch to homing mode
	setOperationMode(OMD_HOMING_MODE);

	// Set homing parameters
	setHomeOffset(offset);
	setSpeedForZeroSearch(10);
	setCurrentThresholdForHomingMode(1500);

	// Display current homing parameters
	std::cout << "Max. Following Error: " << getMaxFollowingError() << std::endl;
	std::cout << "Home Offset: " << getHomeOffset() << std::endl;
	std::cout << "Max. Profile Velocity: " << getMaxProfileVelocity() << std::endl;
	std::cout << "Quick Stop Deceleration: " << getQuickStopDeceleration() << std::endl;
	std::cout << "Speed for Switch Search: " << getSpeedForSwitchSearch() << std::endl;
	std::cout << "Speed for Zero Search: " << getSpeedForZeroSearch() << std::endl;
	std::cout << "Homing Acceleration: " << getHomingAcceleration() << std::endl;
	std::cout << "Current Threshold Homing Mode: " << getCurrentThresholdForHomingMode() << std::endl;
	std::cout << "Home Position: " << getHomePosition() << std::endl;

	// set homing method
	setHomingMethod(method);

	//std::cout << "Shutdown" << std::endl;
	//writeControlword(0x0006);

	// switch on
	std::cout << "Switch-on" << std::endl;
	setControlword(0x000f);

	// start homing mode
	std::cout << "Start homing" << std::endl;
	setControlword(0x001f);

	//read/print status
	monitorHomingStatus();

	WORD w = getStatusWord();
	if ((w & E_BIT13) == E_BIT13) {
		std::cout << "\a *** got a HomingError! ***\n";
		return (-1);
	}

	if ((w & E_BIT12) == E_BIT12) {
		// Just to be sure (this should be OK according to specification)
		if ((w & E_BIT15) != E_BIT15) {
			BOOST_THROW_EXCEPTION(fe() << reason("Not referenced after homing(!?)") << canId(nodeId));
		}
		std::cout << "homing finished!\n";
		return (0);
	} else {
		//  can this be reached? position finished, no homing error but
		//  homing NOT finished? I guess not..
		return (-5);
	}
}

void epos::moveRelative(INTEGER32 steps)
{
	// set the Profile Position Mode
	setOperationMode(OMD_PROFILE_POSITION_MODE);

	// write intended target position
	// firmware 14.1.70
	setTargetPosition(steps);

	// switch to relative positioning BY WRITING TO CONTROLWORD, finish	possible ongoing operation first!
	// see ->maxon applicattion note: device programming 2.1
	startRelativeMotion();
}

void epos::moveAbsolute(INTEGER32 steps)
{
	// set the Profile Position Mode
	setOperationMode(OMD_PROFILE_POSITION_MODE);

	// write intended target position, is signed 32bit int
	// firmware 14.1.70
	setTargetPosition(steps);

	// switch to absolute positioning, cancel possible ongoing operation first!
	// see maxon application note: device programming 2.1
	startAbsoluteMotion();
}

UNSIGNED32 epos::getGearRatioNumerator()
{
	return ReadObjectValue <UNSIGNED32>(0x2230, 0x01);
}

void epos::setGearRatioNumerator(UNSIGNED32 val)
{
	WriteObjectValue(0x2230, 0x01, val);
}

UNSIGNED16 epos::getGearRatioDenominator()
{
	return ReadObjectValue <UNSIGNED16>(0x2230, 0x02);
}

void epos::setGearRatioDenominator(UNSIGNED16 val)
{
	WriteObjectValue(0x2230, 0x02, val);
}

UNSIGNED32 epos::getGearMaximalSpeed()
{
	return ReadObjectValue <UNSIGNED32>(0x2230, 0x03);
}

void epos::setGearMaximalSpeed(UNSIGNED32 val)
{
	WriteObjectValue(0x2230, 0x03, val);
}

// monitor device status
void epos::monitorStatus()
{
	long int postarget, posactual, veldemand, velactual;
	short curactual;
	UNSIGNED16 status;

	printf("\nEPOS operating figures (note: update here is done AS FAST AS POSSIBLE!):\n");
	int i = 0;
	do {
		i++;
		postarget = getTargetPosition();
		posactual = getActualPosition();
		veldemand = setDemandVelocity();
		velactual = getActualVelocity();
		curactual = getActualCurrent();

		printf("\rEPOS: pos=%+10ld |%+10ld (%ld to go); v= %+4ld | %+4ld[rpm]; I=%+4dmA", postarget, posactual, postarget
				- posactual, veldemand, velactual, curactual);
		fflush(stdout);

		status = getStatusWord();
	} while ((status & E_BIT10) != E_BIT10); // bit 10 says: target reached!

	// update values a last time to get a nicer output:
	i++;
	postarget = getTargetPosition();
	posactual = getActualPosition();
	veldemand = setDemandVelocity();
	velactual = getActualVelocity();
	curactual = getActualCurrent();

	printf("\r%d EPOS: pos=%+10ld |%+10ld (%ld to go); v= %+4ld | %+4ld[rpm]; I=%+4dmA\n", i, postarget, posactual, postarget
			- posactual, veldemand, velactual, curactual);
	printf("target reached\n");
}

void epos::Store()
{
	// *save* data
	uint32_t save;
	save = ('s');
	save |= ('a' << 8);
	save |= ('v' << 16);
	save |= ('e' << 24);

	WriteObjectValue(0x1010, 0x01, save);
}

void epos::Restore()
{
	// *load* data
	uint32_t load;
	load = ('l');
	load |= ('o' << 8);
	load |= ('a' << 16);
	load |= ('d' << 24);

	WriteObjectValue(0x1011, 0x01, load);
}


bool epos::isReferenced()
{
	return isReferenced(getStatusWord());
}

bool epos::isReferenced(UNSIGNED16 status)
{
	return (E_BIT15 & status);
}

bool epos::isTargetReached()
{
	return isTargetReached(getStatusWord());
}

bool epos::isTargetReached(UNSIGNED16 status)
{
	return (E_BIT10 & status);
}

bool epos::isFaultState()
{
	return isFaultState(getStatusWord());
}

bool epos::isFaultState(UNSIGNED16 status)
{
	return (E_BIT03 & status);
}

void epos::startHoming()
{
	setControlword(0x001f);
}

bool epos::isHomingFinished()
{
	UNSIGNED16 status = getStatusWord();

	if ((status & E_BIT13) == E_BIT13) {
		BOOST_THROW_EXCEPTION(fe() << reason("HOMING ERROR!") << canId(nodeId));
	}

	// bit 10 says: target reached!, bit 12: homing attained
	return (((status & E_BIT10) == E_BIT10) && ((status & E_BIT12) == E_BIT12));
}

void epos::monitorHomingStatus()
{
	INTEGER32 posactual, velactual;
	INTEGER16 curactual;
	UNSIGNED16 status;
#if 0
	// Recovery retry counter
	unsigned int retry = 0;

	// Recovery status flag
	bool recovered = false;

	// It takes some time to recovery from FAULT state
	while(retry++ < 5) {
		if(getState() == FAULT) {



#endif
	printf("\nEPOS operating figures (note: update here is done AS FAST AS POSSIBLE!):\n");
	int i = 0;

	// Periodic timer
	boost::system_time wakeup = boost::get_system_time();

	do {
		// Wait for device state to change
		boost::thread::sleep(wakeup);

		i++;
		posactual = getActualPosition();
		velactual = getActualVelocity();
		curactual = getActualCurrent();

		status = getStatusWord();

		printf("\r%d EPOS: pos=%+10d; v =%+4drpm I=%+5dmA status = %#06x ", i, posactual, velactual, curactual, status);

		fflush(stdout);

		if ((status & E_BIT13) == E_BIT13) {
			BOOST_THROW_EXCEPTION(fe() << reason("HOMING ERROR") << canId(nodeId));
		}

		// Increment the wakeup time
		wakeup += boost::posix_time::milliseconds(5);

	} while (((status & E_BIT10) != E_BIT10) && ((status & E_BIT12) != E_BIT12));
	// bit 10 says: target reached!, bit 12: homing attained
	//printEPOSstatusword(status);

	i++;
	posactual = getActualPosition();
	velactual = getActualVelocity();
	curactual = getActualCurrent();

	status = getStatusWord();

	printf("\r%d EPOS: pos=%+10d; v =  %+4drpm I=%+4dmA status = %#06x\n", i, posactual, velactual, curactual, status);
	printf("homing finished! Position should now be '0'\n");
}

/* waits for positoning to finish, argument is timeout in
 seconds. give timeout==0 to disable timeout */
int epos::waitForTarget(unsigned int t)
{

	WORD status;
	unsigned int i = 0, st = (unsigned int) 1e4;

	do {
		if (t != 0) { // use timeout?
			if (++i > t * 1e2)
				return (1);
		}
		usleep(st);
		status = getStatusWord();
	} while ((status & E_BIT10) != E_BIT10); // bit 10 says: target reached!

	return (0);
}

INTEGER16 epos::getAnalogInput1() {
	return ReadObjectValue <INTEGER16>(0x207C, 0x01);
}

void epos::configureAnalogInput(int input, analog_input_mode_t mode)
{
	if(input < 0 || input > 2) {
		BOOST_THROW_EXCEPTION(fe() << reason("Analog input line number out of range"));
	}
	WriteObjectValue(0x207B, input, mode);
}

void epos::setAnalogInputFunctionalitiesExecutionMask(bool PositionSetpoint, bool VelocitySetpoint, bool CurrentSetpoint)
{
	UNSIGNED16 mask = 0x0000;

	if(PositionSetpoint) {
		mask |= 0x04;
	}
	if(VelocitySetpoint) {
		mask |= 0x02;
	}
	if(CurrentSetpoint) {
		mask |= 0x01;
	}

	WriteObjectValue(0x207D, 0x00, mask);
}

void epos::setPositionModeSettingValue(INTEGER32 val)
{
	WriteObjectValue(0x2062, 0x00, val);
}

INTEGER32 epos::getPositionModeSettingValue()
{
	return ReadObjectValue <INTEGER32>(0x2062, 0x00);
}

void epos::setAnalogVelocitySetpointScaling(INTEGER16 val)
{
	WriteObjectValue(0x2302, 0x01, val);
}

INTEGER16 epos::getAnalogVelocitySetpointScaling()
{
	return ReadObjectValue <INTEGER16>(0x2302, 0x01);
}

void epos::setAnalogVelocitySetpointOffset(INTEGER32 val)
{
	WriteObjectValue(0x2302, 0x02, val);
}

INTEGER32 epos::getAnalogVelocitySetpointOffset()
{
	return ReadObjectValue <INTEGER32>(0x2302, 0x02);
}

void epos::setAnalogVelocitySetpointNotationIndex(INTEGER8 val)
{
	WriteObjectValue(0x2302, 0x03, val);
}

INTEGER8 epos::getAnalogVelocitySetpointNotationIndex()
{
	return ReadObjectValue <INTEGER8>(0x2302, 0x03);
}

INTEGER32 epos::getAnalogVelocitySetpoint()
{
	return ReadObjectValue <INTEGER32>(0x2302, 0x04);
}

void epos::setDigitalOutputs(digital_outputs_t cmd)
{
	UNSIGNED16 val = 0x0000;

	// Map consecutive bits into general outputs
	if(cmd[0]) val |= (1 << 15);
	if(cmd[1]) val |= (1 << 14);
	if(cmd[2]) val |= (1 << 13);
	if(cmd[3]) val |= (1 << 12);
	if(cmd[4]) val |= (1 << 11);
	if(cmd[5]) val |= (1 << 10);
	if(cmd[6]) val |= (1 << 9);
	if(cmd[7]) val |= (1 << 8);

	WriteObjectValue(0x2078, 0x01, val);

	DigitalOutputs = cmd;
}

UNSIGNED16 epos::getDigitalOutputs()
{
	return ReadObjectValue <UNSIGNED16>(0x2078, 0x01);
}

void epos::setDigitalOutputFunctionalitiesMask(UNSIGNED16 val)
{
	WriteObjectValue(0x2078, 0x02, val);
}

UNSIGNED16 epos::getDigitalOutputFunctionalitiesMask()
{
	return ReadObjectValue <UNSIGNED16>(0x2078, 0x02);
}

void epos::setDigitalOutputFunctionalitiesPolarity(UNSIGNED16 val)
{
	WriteObjectValue(0x2078, 0x03, val);
}

UNSIGNED16 epos::getDigitalOutputFunctionalitiesPolarity()
{
	return ReadObjectValue <UNSIGNED16>(0x2078, 0x03);
}

const epos::digital_outputs_t & epos::getCommandedDigitalOutputs()
{
	return DigitalOutputs;
}

void epos::InitiateSegmentedWrite(WORD index, BYTE subindex, DWORD ObjectLength)
{
	device.InitiateSementedWrite(nodeId, index, subindex, ObjectLength);
}

void epos::SegmentedWrite(BYTE * ptr, std::size_t len)
{
	device.SegmentedWrite(nodeId, ptr, len);
}

/* compare WORD a with WORD b bitwise */
bool epos::bitcmp(WORD a, WORD b)
{
	return ((a & b) == b) ? true : false;
}

//! \@}

} /* namespace maxon */
} /* namespace edp */
} /* namespace mrrocpp */
