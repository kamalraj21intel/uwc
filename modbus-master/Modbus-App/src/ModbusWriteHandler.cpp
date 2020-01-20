/************************************************************************************
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation. Title to the
 * Material remains with Intel Corporation.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or otherwise.
 ************************************************************************************/

#include "Logger.hpp"

#include "ModbusWriteHandler.hpp"
#include "utils/YamlUtil.hpp"
#include <thread>
#include <mutex>
#include "cjson/cJSON.h"
#include "NetworkInfo.hpp"
#include "ZmqHandler.hpp"
#include "ConfigManager.hpp"

#include <sys/msg.h>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include "eis/msgbus/msgbus.h"
#include "eis/utils/json_config.h"

/// stop thread flag
extern std::atomic<bool> g_stopThread;
/// reference if to store request data
std::atomic<unsigned short> refId;

modWriteHandler::modWriteHandler() : m_bIsWriteInitialized(false)
{
	try
	{
		initWriteSem();
		m_bIsWriteInitialized = true;
	}
	catch(const std::exception& e)
	{
		CLogger::getInstance().log(FATAL, LOGDETAILS("Unable to initiate write instance"));
		CLogger::getInstance().log(FATAL, LOGDETAILS(e.what()));
		std::cout << "\nException modWriteHandler ::" << __func__ << ": Unable to initiate instance: " << e.what();
	}
}

bool modWriteHandler::initWriteSem()
{
	//
	int ok = sem_init(&semaphoreWriteReq, 0, 0);
	if (ok == -1) {
	   std::cout << "*******Could not create unnamed write semaphore\n";
	   return false;
	}
	return true;
}

eMbusStackErrorCode modWriteHandler::writeInfoHandler()
{
	CLogger::getInstance().log(DEBUG, LOGDETAILS("Start"));

	stWriteRequest writeReq;
	eMbusStackErrorCode eFunRetType = MBUS_STACK_NO_ERROR;
	unsigned char  m_u8FunCode;
	MbusAPI_t stMbusApiPram = {};

	while(false == g_stopThread.load())
	{
		do
		{
			sem_wait(&semaphoreWriteReq);
			try
			{
				if(false == getWriteDataToProcess(writeReq))
				{
					return MBUS_STACK_ERROR_QUEUE_SEND;
				}

				CLogger::getInstance().log(ERROR, LOGDETAILS(" write Processing initiated :: " + writeReq.m_strMsg));

				if(MBUS_STACK_NO_ERROR == eFunRetType)
				{
					eFunRetType = jsonParserForWrite(writeReq, stMbusApiPram, m_u8FunCode);
				}

				if(MBUS_STACK_NO_ERROR == eFunRetType)
				{
					if(MBUS_MIN_FUN_CODE != m_u8FunCode)
					{
						eFunRetType = (eMbusStackErrorCode) Modbus_Stack_API_Call(
								m_u8FunCode,
								&stMbusApiPram,
								(void*)ModbusMaster_AppCallback);
					}
				}
			}
			catch(const std::exception &e)
			{
				eFunRetType = MBUS_JSON_APP_ERROR_EXCEPTION_RISE;
				CLogger::getInstance().log(FATAL, LOGDETAILS(e.what()));
			}

			if(NULL != stMbusApiPram.m_pu8Data)
			{
				delete[] stMbusApiPram.m_pu8Data;
				stMbusApiPram.m_pu8Data  = NULL;
			}
		}while(0);
	}

	CLogger::getInstance().log(DEBUG, LOGDETAILS("End"));
	return eFunRetType;
}

int char2int(char input)
{
	CLogger::getInstance().log(DEBUG, LOGDETAILS("Start"));
	if(input >= '0' && input <= '9')
		return input - '0';
	if(input >= 'A' && input <= 'F')
		return input - 'A' + 10;
	if(input >= 'a' && input <= 'f')
		return input - 'a' + 10;

	CLogger::getInstance().log(DEBUG, LOGDETAILS("End"));
	throw std::invalid_argument("Invalid input string");
}

int hex2bin(const std::string &src, int iOpLen, uint8_t* target)
{
	CLogger::getInstance().log(DEBUG, LOGDETAILS("Start"));
	int iOpCharPos = 0;
	int i = 0;
	try
	{
		int iLen = src.length();
		if(0 != (iLen%2))
		{
			std::cout << "\n input string is not proper \n";
			return -1;
		}
		// Check if hex string starts with 0x. If yes ignore first 2 letters
		if( ('0' == src[0]) && (('X' == src[1]) || ('x' == src[1])) )
		{
			i = 2;
		}
		iLen = iLen - i;
		if((iLen / 2) != iOpLen)
		{
			std::cout << "\n Input length: " << iLen << ", Output length: " << iOpLen << " Mismatch";
			return -1;
		}
		iLen = iLen + i;
		while (i+1 < iLen)
		{
			unsigned char byte1 = char2int(src[i])*16 + char2int(src[i+1]);
			i = i + 2;
			if ((i+1) < iLen)
			{
				unsigned char byte2 = char2int(src[i])*16 + char2int(src[i+1]);
				target[iOpCharPos] = byte2;
				iOpCharPos++;
			}
			target[iOpCharPos] = byte1;
			iOpCharPos++;
			i = i + 2;
		}
	}
	catch(std::exception &e)
	{
		std::cout << __func__ << ":Exception: " << e.what() << " while parsing hex string " << src << std::endl;
		CLogger::getInstance().log(FATAL, LOGDETAILS(e.what()));
		return -1;
	}
	CLogger::getInstance().log(DEBUG, LOGDETAILS("End"));
	return iOpCharPos;
}

eMbusStackErrorCode modWriteHandler::jsonParserForWrite(stWriteRequest& reqMsg,
											MbusAPI_t &stMbusApiPram,
											unsigned char& funcCode)
{
	CLogger::getInstance().log(DEBUG, LOGDETAILS("Start"));
	eMbusStackErrorCode eFunRetType = MBUS_STACK_NO_ERROR;
	string strCommand, strValue, strWellhead, strVersion, strSourceTopic;
	cJSON *root = cJSON_Parse(reqMsg.m_strMsg.c_str());
	bool isWrite;
	try
	{
		if(MBUS_STACK_NO_ERROR == eFunRetType)
		{
			refId++;
			/// restarting refId once reached max. limit.
			if(refId >= 65535)
				refId = 1;

			stMbusApiPram.m_pu8Data = NULL;
			stMbusApiPram.m_u16TxId = refId;

			cJSON *appseq = cJSON_GetObjectItem(root,"app_seq");
			cJSON *cmd=cJSON_GetObjectItem(root,"command");
			cJSON *value=cJSON_GetObjectItem(root,"value");
			cJSON *wellhead=cJSON_GetObjectItem(root,"wellhead");
			cJSON *version=cJSON_GetObjectItem(root,"version");
			cJSON *sourcetopic=cJSON_GetObjectItem(root,"sourcetopic");
			cJSON *timestamp=cJSON_GetObjectItem(root,"timestamp");
			cJSON *usec=cJSON_GetObjectItem(root,"usec");

			if(cmd && appseq && wellhead && version && timestamp && usec && sourcetopic)
			{
				strCommand = cmd->valuestring;
				strWellhead = wellhead->valuestring;
				strVersion = version->valuestring;
				strSourceTopic = sourcetopic->valuestring;
				//if write then "true" else "false"
				isWrite = strSourceTopic.find("write") <= strSourceTopic.length() ? true : false;
				if(true == isWrite)
					strValue = value->valuestring;
			}
			else
			{
				std::cout << __func__ << " Invalid input json parameter." << std::endl;
				CLogger::getInstance().log(ERROR, LOGDETAILS(" Invalid input json parameter."));
				eFunRetType = MBUS_JSON_APP_ERROR_INVALID_INPUT_PARAMETER;
			}

			string strSearchString = "/";
			std::size_t found1 = strSourceTopic.find(strSearchString);
			std::size_t found2 = strSourceTopic.find(strSearchString.c_str(), found1+1);
			string stTopic = strWellhead +
					SEPARATOR_CHAR +
					strSourceTopic.substr(found1+1, found2-found1-1) +
					SEPARATOR_CHAR +
					strCommand;

			std::map<std::string, network_info::CUniqueDataPoint> mpp = network_info::getUniquePointList();
			struct network_info::stModbusAddrInfo addrInfo = mpp.at(stTopic).getWellSiteDev().getAddressInfo();

			network_info::CDataPoint obj = mpp.at(stTopic).getDataPoint();
#ifdef MODBUS_STACK_TCPIP_ENABLED
			string stIpAddress = addrInfo.m_stTCP.m_sIPAddress;
			stMbusApiPram.m_u16Port = addrInfo.m_stTCP.m_ui16PortNumber;
			stMbusApiPram.m_u8DevId = addrInfo.m_stTCP.m_uiUnitID;
			CommonUtils::ConvertIPStringToCharArray(stIpAddress,&(stMbusApiPram.m_u8IpAddr[0]));
#else
			stMbusApiPram.m_u8DevId = addrInfo.m_stRTU.m_uiSlaveId;
#endif

			stMbusApiPram.m_u16StartAddr = (uint16_t)obj.getAddress().m_iAddress;
			stMbusApiPram.m_u16Quantity = (uint16_t)obj.getAddress().m_iWidth;
			network_info::eEndPointType eType = obj.getAddress().m_eType;

			switch(eType)
			{
			case network_info::eEndPointType::eCoil:
				funcCode = isWrite ? stMbusApiPram.m_u16Quantity == 1 ? WRITE_SINGLE_COIL: WRITE_MULTIPLE_COILS : READ_COIL_STATUS;
				break;
			case network_info::eEndPointType::eHolding_Register:
				funcCode = isWrite ? stMbusApiPram.m_u16Quantity == 1 ? WRITE_SINGLE_REG: WRITE_MULTIPLE_REG : READ_HOLDING_REG;
				break;
			case network_info::eEndPointType::eInput_Register:
				funcCode = READ_INPUT_REG;
				break;
			case network_info::eEndPointType::eDiscrete_Input:
				funcCode = READ_INPUT_STATUS;
				break;
			default:
				CLogger::getInstance().log(ERROR, LOGDETAILS(" Invalid type in datapoint:: " + strCommand));
				break;
			}

			if(NULL != appseq)
			{
				stOnDemandRequest reqData;
				reqData.m_strAppSeq = appseq->valuestring;
				reqData.m_isByteSwap = obj.getAddress().m_bIsByteSwap;
				reqData.m_isWordSwap = obj.getAddress().m_bIsWordSwap;
				reqData.m_strMetric = strCommand;
				reqData.m_strVersion = strVersion;
				reqData.m_strWellhead = strWellhead;
				zmq_handler::insertOnDemandReqData(refId, reqData);
			}

			if(WRITE_MULTIPLE_REG == funcCode)
			{
				stMbusApiPram.m_u16ByteCount = stMbusApiPram.m_u16Quantity*2;
			}
			else if(WRITE_MULTIPLE_COILS == funcCode)
			{
				uint8_t u8ByteCount = (0 != (stMbusApiPram.m_u16Quantity%8))
											?((stMbusApiPram.m_u16Quantity/8)+1)
											:(stMbusApiPram.m_u16Quantity/8);

				stMbusApiPram.m_u16ByteCount = (uint8_t)u8ByteCount;
			}
			else if(WRITE_SINGLE_COIL == funcCode ||
					WRITE_SINGLE_REG == funcCode)
			{
				stMbusApiPram.m_u16ByteCount = MODBUS_SINGLE_REGISTER_LENGTH;
			}
			stMbusApiPram.m_pu8Data = new uint8_t[stMbusApiPram.m_u16ByteCount]();
			if(NULL != stMbusApiPram.m_pu8Data)
			{
				if(WRITE_SINGLE_COIL == funcCode)
				{
					// If value is 0x01, then write 0xFF00
					if( (0 == strValue.compare("0x00")) || (0 == strValue.compare("0X00"))
							|| (0 == strValue.compare("00")) || (0 == strValue.compare("0")))
					{
						strValue = "0x0000";
					}
					else
					{
						strValue = "0xFF00";
					}
				}
				if(eType != network_info::eEndPointType::eCoil &&
						eType != network_info::eEndPointType::eDiscrete_Input)
				{
					if((true == obj.getAddress().m_bIsByteSwap || true == obj.getAddress().m_bIsWordSwap) &&
							true == isWrite)
					{
						std::vector<uint8_t> tempVt;
						int i = 0;
						if( ('0' == strValue[0]) && (('X' == strValue[1]) || ('x' == strValue[1])) )
						{
							i = 2;
						}
						int iLen = strValue.length();
						while(i < iLen)
						{
							unsigned char byte1 = char2int(strValue[i])*16 + char2int(strValue[i+1]);
							tempVt.push_back(byte1);
							i = i+2;
						}
						strValue = zmq_handler::swapConversion(tempVt,
								!obj.getAddress().m_bIsByteSwap,
								obj.getAddress().m_bIsWordSwap);
					}
					hex2bin(strValue, stMbusApiPram.m_u16ByteCount, stMbusApiPram.m_pu8Data);
				}
				/*else
				{
					char* token = strtok(strValue.c_str(), ",");
					for (unsigned int i= 0 ; i< stMbusApiPram.m_u16ByteCount && NULL != token; ++i)
					{
						uint32_t val = stoi(token);
						stMbusApiPram.m_pu8Data[i] = (uint8_t)val;
						token = strtok(NULL, ",");
					}
				}*/
			}
			else
			{
				eFunRetType = MBUS_JSON_APP_ERROR_EXCEPTION_RISE;
				CLogger::getInstance().log(ERROR, LOGDETAILS(" Unable to allocate memory. Request not sent"));
			}
		}
	}
	catch(const std::exception &e)
	{
		eFunRetType = MBUS_JSON_APP_ERROR_EXCEPTION_RISE;
		CLogger::getInstance().log(FATAL, LOGDETAILS(e.what()));
	}

	if(NULL != root)
		cJSON_Delete(root);

	CLogger::getInstance().log(DEBUG, LOGDETAILS("End"));
	return eFunRetType;
}

//TODO function name change
void modWriteHandler::createWriteListener()
{
	CLogger::getInstance().log(DEBUG, LOGDETAILS("Start"));
	if(getenv("SubTopics") == NULL)
	{
		cout<< __func__<<" Error:: SubTopics are not configured.." << endl;
		return;
	}

	/// parse all the subtopics
	std::vector<std::string> stTopics = CfgManager::Instance().getEnvConfig().get_topics_from_env("sub");

	if(stTopics.empty())
	{
		std::cout << __func__ << " sub topic is not available. " << std::endl;
		CLogger::getInstance().log(INFO, LOGDETAILS("No subscribe topic is available."));

		return;
	}

	for(auto sTopic : stTopics)
	{
		std::size_t pos = sTopic.find('/');
		if (std::string::npos != pos)
		{
			std::string subTopic(sTopic.substr(pos + 1));
			std::thread(&modWriteHandler::subscribeDeviceListener, this, subTopic).detach();
		}
		else
		{
			CLogger::getInstance().log(ERROR, LOGDETAILS("Incorrect topic name format:" + sTopic));
		}
	}
	CLogger::getInstance().log(DEBUG, LOGDETAILS("End"));
}

modWriteHandler& modWriteHandler::Instance()
{
	static modWriteHandler _self;
	return _self;
}

void modWriteHandler::subscribeDeviceListener(const std::string stTopic)
{
	msg_envelope_t *msg = NULL;
	msg_envelope_serialized_part_t* parts = NULL;
	int num_parts = 0;
	msgbus_ret_t ret;
	try
	{
		zmq_handler::stZmqContext msgbus_ctx = zmq_handler::getCTX(stTopic);
		zmq_handler::stZmqSubContext stsub_ctx = zmq_handler::getSubCTX(stTopic);

		while((msgbus_ctx.m_pContext != NULL) && (NULL != stsub_ctx.sub_ctx)
				&& (false == g_stopThread.load()))
		{
			ret = msgbus_recv_wait(msgbus_ctx.m_pContext, stsub_ctx.sub_ctx, &msg);
			if(ret != MSG_SUCCESS)
			{
				CLogger::getInstance().log(ERROR, LOGDETAILS("Failed to receive message errno ::" + std::to_string(ret)));
				continue;
			}

			num_parts = msgbus_msg_envelope_serialize(msg, &parts);
			if(num_parts <= 0)
			{

				CLogger::getInstance().log(ERROR, LOGDETAILS(
						" Failed to serialize message"));
			}

			if(NULL != parts[0].bytes)
			{
				struct stWriteRequest stWriteRequestNode;
				std::string strMsg(parts[0].bytes);

				stWriteRequestNode.m_strTopic = stTopic;
				stWriteRequestNode.m_strMsg = strMsg;
				string initiate = " write initiated for msg:: ";
				initiate.append(strMsg);

				CLogger::getInstance().log(ERROR, LOGDETAILS("initiate"));
				/// pushing write request to q to process.
				pushToWriteTCPQueue(stWriteRequestNode);
			}

			msgbus_msg_envelope_serialize_destroy(parts, num_parts);
			msgbus_msg_envelope_destroy(msg);
			msg = NULL;
			parts = NULL;

			if(parts != NULL)
				msgbus_msg_envelope_serialize_destroy(parts, num_parts);
		}
	}
	catch(const std::exception& e)
	{
		CLogger::getInstance().log(FATAL, LOGDETAILS(e.what()));
		std::cout << "\nException subscribeDeviceListener ::" << __func__ << e.what();
	}
}

bool modWriteHandler::pushToWriteTCPQueue(struct stWriteRequest &stWriteRequestNode)
{
	try
	{
		std::lock_guard<std::mutex> lock(__writeReqMutex);
		stackTCPWriteReqQ.push(stWriteRequestNode);
		// Signal thread to process
		sem_post(&semaphoreWriteReq);

		return true;
	}
	catch(const std::exception& e)
	{
		CLogger::getInstance().log(FATAL, LOGDETAILS(e.what()));
	}

	return false;
}

void modWriteHandler::initWriteHandlerThreads()
{
	static bool bWriteSpawned = false;
	try
	{
		if(false == bWriteSpawned)
		{
			// Spawn 5 thread to process write request
			//for (int i = 0; i < 5; i++)
			{
				std::thread{std::bind(&modWriteHandler::writeInfoHandler, std::ref(*this))}.detach();
			}
			bWriteSpawned = true;
		}
	}
	catch(const std::exception& e)
	{
		CLogger::getInstance().log(FATAL, LOGDETAILS(e.what()));
		std::cout << "\nException modWriteHandler ::" << __func__ << ": Unable to initiate write handler instance: " << e.what();
	}
}

bool modWriteHandler::getWriteDataToProcess(struct stWriteRequest &stWriteProcessNode)
{
	CLogger::getInstance().log(ERROR, LOGDETAILS("Start"));
	try
	{
		std::lock_guard<std::mutex> lock(__writeReqMutex);
		stWriteProcessNode = stackTCPWriteReqQ.front();
		stackTCPWriteReqQ.pop();

		return true;
	}
	catch(const std::exception& e)
	{
		CLogger::getInstance().log(FATAL, LOGDETAILS(e.what()));
	}

	CLogger::getInstance().log(ERROR, LOGDETAILS("End"));
	return false;
}
