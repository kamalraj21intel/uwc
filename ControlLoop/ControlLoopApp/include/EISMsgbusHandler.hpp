/*************************************************************************************
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Intel Corporation. Title to the
* Material remains with Intel Corporation.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or delivery of
* the Materials, either expressly, by implication, inducement, estoppel or otherwise.
*************************************************************************************/

#ifndef INCLUDE_EISMSGBUSHANDLER_HPP_
#define INCLUDE_EISMSGBUSHANDLER_HPP_

#include <string>
#include "eis/msgbus/msgbus.h"
#include "ConfigManager.hpp"

struct stZmqContext
{
	void *m_pContext;
};
struct stZmqSubContext
{
	recv_ctx_t *m_pContext;
};

struct stZmqPubContext
{
	void *m_pContext;
};

/*
 * CEISMsgbusHandler to receive topic-msg from ZMQ and send to MQTT
 */
class CEISMsgbusHandler {

	CEISMsgbusHandler();
	CEISMsgbusHandler(const CEISMsgbusHandler &obj);
	CEISMsgbusHandler operator=(const CEISMsgbusHandler &obj);
	std::vector<std::string> subTopicList;

public:
	static CEISMsgbusHandler& Instance() {
		static CEISMsgbusHandler _self;
			return _self;
	}

	//CEISMsgbusHandler();
	virtual ~CEISMsgbusHandler();

	void cleanup();

	/**
	 * Prepare all EIS contexts for zmq communications based on topic configured in
	 * SubTopics or PubTopics section from docker-compose.yml file
	 * Following is the sequence of context creation
	 * 	1. Get the topic from SubTopics/PubTopics section
	 * 	2. Create msgbus config
	 * 	3. Create the msgbus context based on msgbus config
	 * 	4. Once msgbus context is successful then create pub and sub context for zmq publisher/subscriber
	 *
	 * @param topicType	:[in] topic type to create context for, value is either "sub" or "pub"
	 * @return 	true : on success,
	 * 			false : on error
	 */
	bool prepareCommonContext(std::string topicType);

	/**
	 * Prepare pub or sub context for ZMQ communication
	 * @param a_bIsPub		:[in] flag to check for Pub or Sub
	 * @param msgbus_ctx	:[in] Common message bus context used for zmq communication
	 * @param a_sTopic		:[in] Topic for which pub or sub context needs to be created
	 * @param config		:[in] Config instance used for zmq library
	 * @return 	true : on success,
	 * 			false : on error
	 */
	bool prepareContext(bool a_bIsPub,
			void* msgbus_ctx,
			string a_sTopic,
			config_t *config);

	// function to get message bus context based on topic
	bool getCTX(std::string a_sTopic, stZmqContext& msgbusContext);

	// function to insert new entry in map
	bool insertCTX(std::string, stZmqContext );

	// function to remove entry from the map once reply is sent
	void removeCTX(std::string);

	// function to get message bus publish context based on topic
	bool getSubCTX(std::string a_sTopic, stZmqSubContext& subContext);

	// function to insert new entry in map
	bool insertSubCTX(std::string, stZmqSubContext );

	// function to remove entry from the map
	void removeSubCTX(std::string, stZmqContext& zmqCtx);

	bool getPubCTX(std::string a_sTopic, stZmqPubContext& pubContext);

	// function to insert new entry in map
	bool insertPubCTX(std::string, stZmqPubContext);

	// function to remove entry from the map
	void removePubCTX(std::string, stZmqContext& zmqCtx);

	const std::vector<std::string>& getSubTopicList() const {
		return subTopicList;
	}

	void insertSubTopicInList(const std::string a_sTopic) {
		subTopicList.push_back(a_sTopic);
	}
};

#endif