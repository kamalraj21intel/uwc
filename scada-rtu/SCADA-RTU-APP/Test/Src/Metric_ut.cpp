/************************************************************************************
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Intel Corporation. Title to the
* Material remains with Intel Corporation.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or delivery of
* the Materials, either expressly, by implication, inducement, estoppel or otherwise.
************************************************************************************/

#include "../Inc/Metric_ut.hpp"

void Metric_ut::SetUp()
{
	// Setup code
}

void Metric_ut::TearDown()
{
	// TearDown code
}

TEST_F(Metric_ut, assignToSparkPlug_UnsupportedDataType)
{
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_BOOLEAN)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_BOOLEAN, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}
#if 0 // In progress
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_UINT16)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT16, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_UINT8)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT8, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_INT8)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_INT8, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_INT16)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_INT16, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_INT32)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_INT32, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_UINT32)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT32, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_UINT64)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT64, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_INT64)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_INT64, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_FLOAT)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_FLOAT, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_DOUBLE)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_DOUBLE, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}

TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_STRING)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_STRING, CValObj_ins);
	EXPECT_EQ( true, CValObj_main.assignToSparkPlug(a_metric) );
}
#endif