/*
 * Author: Neuber Sousa <neuberfran@gmail.com>
 * Below are the terms of usage of this file
 *
 * This is an upm implementation for SmartDrive from OpenElectrons.Com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <exception>
#include <stdexcept>
#include <unistd.h>
#include <stdlib.h>

#include "smartdrive.h"


using namespace upm;

SmartDrive::SmartDrive(int i2c_bus, int address): m_controlAddr(address), m_i2ControlCtx(i2c_bus)
{
    mraa::Result ret = m_i2ControlCtx.address(m_controlAddr);
    if (ret != mraa::SUCCESS) {
        throw std::invalid_argument(std::string(__FUNCTION__) +
                                    ": mraa_i2c_address() failed");
        return;
    }
}


void
SmartDrive::writeByte(uint8_t addr, uint8_t value) {
	try {
		m_i2ControlCtx.address(m_controlAddr);
		m_i2ControlCtx.writeReg(addr, value);
	} catch (int e) {
		std::cout << "Failed to write " << value << " to address " << addr << " --> " << e << std::endl;
	}
}

uint8_t
SmartDrive::readByte(uint8_t addr) {
	try {
		m_i2ControlCtx.address(m_controlAddr);
		return m_i2ControlCtx.readReg(addr);
	} catch (int e) {
		std::cout << "Failed to read byte at address " << addr << " --> " << e << std::endl;
	}
	return -1;
}

void
SmartDrive::writeArray(uint8_t* array, int size) {
	try {
		m_i2ControlCtx.address(m_controlAddr); //Second : I was not resetting bus adress of device evreytime ebfore acessing it
											   //I believe we need to do this everytime before accessing bus, because i2c bus may be used by many devices, se we need to tell which device we want to control EVERYTIME
		m_i2ControlCtx.write(array, size); //First : Here, i was tryignt o calculate array size dynamically, but it's not supported here, so i have to pass array size statically in variable
	} catch (int e) {
		std::cout << "Failed to write array values to address " << array[0] << " --> " << e << std::endl;
	}
}

uint16_t
SmartDrive::readInteger(uint8_t addr) {
	try {
		m_i2ControlCtx.address(m_controlAddr);
		return m_i2ControlCtx.readWordReg(addr);
	} catch (int e) {
		std::cout << "Failed to read value at address " << addr << " --> " << e << std::endl;
	}
	return -1;
}

uint32_t
SmartDrive::readLongSigned(uint8_t addr) {
	uint8_t bytes[4]={0};

	try {
		m_i2ControlCtx.address(m_controlAddr);
		m_i2ControlCtx.readBytesReg(addr, bytes, sizeof(bytes)/sizeof(uint8_t)); 
		return (bytes[0]|(bytes[1]<<8)|(bytes[2]<<16)|(bytes[3]<<24));
	} catch (int e) {
		std::cout << "Failed to read integer value at address " << addr << " --> " << e << std::endl;
	}
	return -1;
}

void
SmartDrive::command(uint8_t cmd) {
    std::cout << "Running Command : " << cmd << std::endl;
    writeByte(SmartDrive_COMMAND, cmd);
}


float
SmartDrive::GetBattVoltage() {
	uint8_t value = 0;
	try {
		value = readByte(SmartDrive_BATT_VOLTAGE);
		return (value * SmartDrive_VOLTAGE_MULTIPLIER);
	} catch (int e) {
        std::cout << "Error: Could not read voltage -> " << e << std::endl;
    }
    return -1.0f;
}


uint32_t
SmartDrive::ReadTachometerPosition(MotorID_t motor_number) {
    try {
        if (motor_number == 1 )
            return readLongSigned(SmartDrive_POSITION_M1);
        else
            return readLongSigned(SmartDrive_POSITION_M2);
	} catch (int e) {
        std::cout << "Error: Could not read encoders" << std::endl;
	}
	return -1;
}


void
SmartDrive::Run_Unlimited(MotorID_t motor_number, Direction_t direction, uint8_t speed) {
        uint8_t ctrl = 0;
        ctrl |= SmartDrive_CONTROL_SPEED;
        ctrl |= SmartDrive_CONTROL_BRK;

        std::cout << "Running with speed : " << (int) speed << std::endl;
        
        if ( motor_number != Motor_ID_BOTH )
            ctrl |= SmartDrive_CONTROL_GO;
        if ( direction != Dir_Forward )
            speed = speed * -1;
        if ( motor_number != Motor_ID_2) {
            uint8_t array [5] = {SmartDrive_SPEED_M1, speed, 0, 0, ctrl};
            writeArray(array,5);
		}
        if ( motor_number != Motor_ID_1) {
            uint8_t array [5] = {SmartDrive_SPEED_M2, speed, 0, 0, ctrl};
            writeArray(array,5); 
		}
        if ( motor_number == Motor_ID_BOTH )
            writeByte(SmartDrive_COMMAND,0x53);
}


void
SmartDrive::StopMotor(MotorID_t motor_number, MotorAction_t next_action ) {
        if ( next_action != Action_Float )
            writeByte(SmartDrive_COMMAND, 'A'+motor_number-1);
        else
            writeByte(SmartDrive_COMMAND, 'a'+motor_number-1);
}


void
SmartDrive::Run_Seconds(MotorID_t motor_number, Direction_t direction, uint8_t speed, uint8_t duration, bool wait_for_completion, MotorAction_t next_action ) {
        uint8_t ctrl = 0;
        ctrl |= SmartDrive_CONTROL_SPEED;
        ctrl |= SmartDrive_CONTROL_TIME;

        if ( next_action == Action_Brake )
            ctrl |= SmartDrive_CONTROL_BRK;
        if ( next_action == Action_BrakeHold ) {
            ctrl |= SmartDrive_CONTROL_BRK;
            ctrl |= SmartDrive_CONTROL_ON;
		}
        if ( motor_number != Motor_ID_BOTH )
            ctrl |= SmartDrive_CONTROL_GO;
        if ( direction != Dir_Forward )
            speed = speed * -1;
        if ( (motor_number & 0x01) != 0 ) {
            uint8_t array[5] = {SmartDrive_SPEED_M1, speed, duration, 0, ctrl};
			printf("Motor 1 running | cmd = %2x %2x %2x %2x %2x\n", array[0] ,array[1] ,array[2] ,array[3] , array[4]);
            writeArray(array,5);
		}
        if ( (motor_number & 0x02) != 0 ) {
            uint8_t array[5] = {SmartDrive_SPEED_M2, speed, duration, 0, ctrl};
			printf("Motor 2 running | cmd = %2x %2x %2x %2x %2x\n", array[0] ,array[1] ,array[2] ,array[3] , array[4]);
            writeArray(array,5);
		}
        if ( motor_number == Motor_ID_BOTH )
            writeByte(SmartDrive_COMMAND, 0x53);      
		printf("Speed Motor 1 : %2x\n", readByte(SmartDrive_SPEED_M1));
		printf("Speed Motor 2 : %2x\n", readByte(SmartDrive_SPEED_M2));
		printf("Time Motor 1 : %2x\n", readByte(SmartDrive_TIME_M1));
		printf("Time Motor 2 : %2x\n", readByte(SmartDrive_TIME_M2));
        if ( wait_for_completion ) {
            sleep(1); //this delay is required for the status byte to be available for reading.
            WaitUntilTimeDone(motor_number);
		}
}


void
SmartDrive::WaitUntilTimeDone(MotorID_t motor_number) {
        while (IsTimeDone(motor_number) == false)
            sleep(1);       
}

   
bool
SmartDrive::IsTimeDone(MotorID_t motor_number) {
		uint8_t result_1 = 0, result_2 = 0;
        if ( motor_number != Motor_ID_2 )
            result_1 = readByte(SmartDrive_STATUS_M1);
		if ( motor_number != Motor_ID_1 )
            result_2 = readByte(SmartDrive_STATUS_M2);
        return (((result_1 & 0x40) == 0) && ((result_2 & 0x40) == 0) );  //look for time bits to be zero
}


void
SmartDrive::Run_Degrees(MotorID_t motor_number, Direction_t direction, uint8_t speed, uint32_t degrees, bool wait_for_completion, MotorAction_t next_action) {
        uint8_t ctrl = 0;
        ctrl |= SmartDrive_CONTROL_SPEED;
        ctrl |= SmartDrive_CONTROL_TACHO;
        ctrl |= SmartDrive_CONTROL_RELATIVE;

        uint32_t d = degrees;
        if ( direction != Dir_Forward )
            d = degrees * -1 ;
        
        uint8_t t4 = (d/0x1000000);
        uint8_t t3 = ((d%0x1000000)/0x10000);
        uint8_t t2 = (((d%0x1000000)%0x10000)/0x100);
        uint8_t t1 = (((d%0x1000000)%0x10000)%0x100);
        
        if ( next_action == Action_Brake )
            ctrl |= SmartDrive_CONTROL_BRK;
        if ( next_action == Action_BrakeHold ) {
            ctrl |= SmartDrive_CONTROL_BRK;
            ctrl |= SmartDrive_CONTROL_ON;
		}
        if ( motor_number != Motor_ID_BOTH )
            ctrl |= SmartDrive_CONTROL_GO;     
        if ( motor_number != Motor_ID_2) {
            uint8_t array[9] = {SmartDrive_SETPT_M1, t1, t2, t3, t4, speed, 0, 0, ctrl};
            writeArray(array,9);
		}
        if ( motor_number != Motor_ID_1){
            uint8_t array[9] = {SmartDrive_SETPT_M2, t1, t2, t3, t4, speed, 0, 0, ctrl};
            writeArray(array,9);
		}
        if ( motor_number == Motor_ID_BOTH )
            writeByte(SmartDrive_COMMAND, 0x53);         
        if ( wait_for_completion ) {
            sleep(1);//this delay is required for the status byte to be available for reading.
            WaitUntilTachoDone(motor_number);
        }
}


void
SmartDrive::Run_Rotations(MotorID_t motor_number, Direction_t direction, uint8_t speed, uint32_t rotations, bool wait_for_completion, MotorAction_t next_action) {
        uint8_t ctrl = 0;
        ctrl |= SmartDrive_CONTROL_SPEED;
        ctrl |= SmartDrive_CONTROL_TACHO;
        ctrl |= SmartDrive_CONTROL_RELATIVE;
        
        uint32_t d = rotations * 360;
        if ( direction != Dir_Forward )
            d = (rotations * 360) * -1;
        
        uint8_t t4 = (d/0x1000000);
        uint8_t t3 = ((d%0x1000000)/0x10000);
        uint8_t t2 = (((d%0x1000000)%0x10000)/0x100);
        uint8_t t1 = (((d%0x1000000)%0x10000)%0x100);
        
        if ( next_action == Action_Brake )
            ctrl |= SmartDrive_CONTROL_BRK;
        if ( next_action == Action_BrakeHold ) {
            ctrl |= SmartDrive_CONTROL_BRK;
            ctrl |= SmartDrive_CONTROL_ON;
		}
        if ( motor_number != Motor_ID_BOTH )
            ctrl |= SmartDrive_CONTROL_GO;
        if ( motor_number != Motor_ID_2) {
            uint8_t array[9] = {SmartDrive_SETPT_M1, t1, t2, t3, t4, speed, 0, 0, ctrl};
            writeArray(array,9);
		}
        if ( motor_number != Motor_ID_1) {
            uint8_t array[9] = {SmartDrive_SETPT_M2, t1, t2, t3, t4, speed, 0, 0, ctrl};
            writeArray(array,9);
		}
        if ( motor_number == Motor_ID_BOTH )
            writeByte(SmartDrive_COMMAND, 0x53);
        if ( wait_for_completion) {
            sleep(1); //this delay is required for the status byte to be available for reading.
            WaitUntilTachoDone(motor_number);
		}
}


void
SmartDrive::Run_Tacho(MotorID_t motor_number, uint8_t speed, uint32_t tacho_count, bool wait_for_completion, MotorAction_t next_action) {
        uint8_t ctrl = 0;
        ctrl |= SmartDrive_CONTROL_SPEED;
        ctrl |= SmartDrive_CONTROL_TACHO;
        
        uint32_t d = tacho_count;
        
        uint8_t t4 = (d/0x1000000);
        uint8_t t3 = ((d%0x1000000)/0x10000);
        uint8_t t2 = (((d%0x1000000)%0x10000)/0x100);
        uint8_t t1 = (((d%0x1000000)%0x10000)%0x100);
        
        if ( next_action == Action_Brake )
            ctrl |= SmartDrive_CONTROL_BRK;
        if ( next_action == Action_BrakeHold ) {
            ctrl |= SmartDrive_CONTROL_BRK;
            ctrl |= SmartDrive_CONTROL_ON;
		}
        if ( motor_number != Motor_ID_BOTH )
            ctrl |= SmartDrive_CONTROL_GO;
        if ( motor_number != Motor_ID_2){
            uint8_t array[9]= {SmartDrive_SETPT_M1, t1, t2, t3, t4, speed, 0, 0, ctrl};
            writeArray(array,9);
		}
        if ( motor_number != Motor_ID_1){
            uint8_t array[9]= {SmartDrive_SETPT_M2, t1, t2, t3, t4, speed, 0, 0, ctrl};
            writeArray(array,9);
		}
        if ( motor_number == Motor_ID_BOTH )
            writeByte(SmartDrive_COMMAND, 0x53);  
        if ( wait_for_completion )
            sleep(1); //this delay is required for the status byte to be available for reading.
            WaitUntilTachoDone(motor_number);
}


void
SmartDrive::WaitUntilTachoDone(MotorID_t motor_number) {
        while (IsTachoDone(motor_number) == false)
            usleep(30000);
}


bool
SmartDrive::IsTachoDone(MotorID_t motor_number) {
		uint8_t result_1 = 0, result_2 = 0;

        if ( motor_number != Motor_ID_2 )
            result_1 = readByte(SmartDrive_STATUS_M1);     
        if ( motor_number != Motor_ID_1 )
            result_2 = readByte(SmartDrive_STATUS_M2);
        //look for both time bits to be zero
        return (((result_1 & 0x08) == 0) && ((result_2 & 0x08) == 0) );
}


void
SmartDrive::SetPerformanceParameters( uint16_t Kp_tacho, uint16_t Ki_tacho, uint16_t Kd_tacho, uint16_t Kp_speed, uint16_t Ki_speed, uint16_t Kd_speed, uint8_t passcount, uint8_t tolerance) {
    uint8_t Kp_t1 = Kp_tacho%0x100;
    uint8_t Kp_t2 = Kp_tacho/0x100;    
    uint8_t Ki_t1 = Ki_tacho%0x100;
    uint8_t Ki_t2 = Ki_tacho/0x100;
    uint8_t Kd_t1 = Kd_tacho%0x100;
    uint8_t Kd_t2 = Kd_tacho/0x100;
    uint8_t Kp_s1 = Kp_speed%0x100;       
    uint8_t Kp_s2 = Kp_speed/0x100;
    uint8_t Ki_s1 = Ki_speed%0x100; 
    uint8_t Ki_s2 = Ki_speed/0x100;
    uint8_t Kd_s1 = Kd_speed%0x100;
    uint8_t Kd_s2 = Kd_speed/0x100;

    uint8_t array[15] = {SmartDrive_P_Kp, Kp_t1 , Kp_t2 , Ki_t1, Ki_t2, Kd_t1, Kd_t2, Kp_s1, Kp_s2, Ki_s1, Ki_s2, Kd_s1, Kd_s2, passcount, tolerance};
    writeArray(array,15);
}


void
SmartDrive::ReadPerformanceParameters() {
   try {
        std::cout << "Pkp: " << readInteger(SmartDrive_P_Kp) << std::endl;
        std::cout << "Pki: " << readInteger(SmartDrive_P_Ki) << std::endl;
        std::cout << "Pkd: " << readInteger(SmartDrive_P_Kd) << std::endl;
        std::cout << "Skp: " << readInteger(SmartDrive_S_Kp) << std::endl;
        std::cout << "Ski: " << readInteger(SmartDrive_S_Ki) << std::endl;
        std::cout << "Skd: " << readInteger(SmartDrive_S_Kd) << std::endl;
        std::cout << "Passcount: " << SmartDrive_PASSCOUNT << std::endl; //ToDo : Check if these should actually be Register Reads !!
        std::cout << "Tolerance: " << SmartDrive_PASSTOLERANCE << std::endl;
    } catch( int e) {
        std::cout << "Error: Could not read PID values -> " << e << std::endl;
	}
}

uint8_t
SmartDrive::GetMotorStatus(MotorID_t motor_id) {
	uint8_t status=0;
	if (motor_id == Motor_ID_1)
		status = readByte(SmartDrive_STATUS_M1);
	if (motor_id == Motor_ID_2)
		status = readByte(SmartDrive_STATUS_M1);
	if (motor_id == Motor_ID_BOTH) {
		std::cout << "Please specifiy which motor's status you want to fetch !" << std::endl;
	}
	return status;
}

void
SmartDrive::PrintMotorStatus(MotorID_t motor_id) {
	if (motor_id != Motor_ID_BOTH) {
	uint8_t status = GetMotorStatus(motor_id);
	uint8_t control_on = (status & SmartDrive_MOTOR_CONTROL_ON);
	uint8_t is_ramping = (status & SmartDrive_MOTOR_IS_RAMPING);
	uint8_t is_powered = (status & SmartDrive_MOTOR_IS_POWERED);
	uint8_t pos_control_on = (status & SmartDrive_MOTOR_POS_CTRL_ON);
	uint8_t in_brake_mode = (status & SmartDrive_MOTOR_IN_BRAKE_MODE);
	uint8_t is_overloaded = (status & SmartDrive_MOTOR_OVERLOADED);
	uint8_t in_time_mode = (status & SmartDrive_MOTOR_IN_TIME_MODE);
	uint8_t is_stalled = (status & SmartDrive_MOTOR_IS_STALLED);

	std::cout << "Motor " << motor_id+1 << " is programemd to move at " << ( (control_on == 0) ? "variable" : "fixed") << " speed" << std::endl;
	std::cout << "Motor " << motor_id+1 << " is " << ((is_ramping == 0) ? "NOT" : "") << " ramping" << std::endl;
	std::cout << "Motor " << motor_id+1 << " is " <<  ((is_powered == 0) ? "NOT" : "") << " powered" << std::endl;
	std::cout << "Motor " << motor_id+1 << " is " <<  ((pos_control_on == 0) ? "moving towards desired encoder " : "holding it ") << "position" << std::endl;
	std::cout << "Motor " << motor_id+1 << " is in " <<  ((in_brake_mode == 0) ? "brake" : "float") << " mode" << std::endl;
	std::cout << "Motor " << motor_id+1 << " is " <<  ((is_overloaded == 0) ? "NOT" : "") << " overloaded" << std::endl;
	std::cout << "Motor " << motor_id+1 << " is " <<  ((in_time_mode == 0) ? "NOT" : "") << " in time mode" << std::endl;
	std::cout << "Motor " << motor_id+1 << " is " <<  ((is_stalled == 0) ? "NOT" : "") << " stalled" << std::endl;

	} else {
		std::cout << "Please specifiy which motor's status you want to fetch !" << std::endl;
	}
}
