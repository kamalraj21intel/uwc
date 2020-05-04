/************************************************************************************
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Intel Corporation or Softdel Systems
* (and licensed to Intel Corporation). Title to the Material remains with
* Intel Corporation or Softdel Systems.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or delivery of
* the Materials, either expressly, by implication, inducement, estoppel or otherwise.
************************************************************************************/

#include <stdbool.h>
#include "osalLinux.h"
#include <stdio.h>
#include <sched.h>

/**
 *
 * DESCRIPTION
 * The OSAL API allocates memory from the heap of specified size.
 *
 * @param a_i32MemorySize [in] int32_t memory size to be allocated.
 *
 * @return [out] void* Allocated memory location
 *
 */
void *OSAL_Malloc(int32_t a_i32MemorySize)
{
    void *pvData = NULL;

    // Allocate memory of requested size
    pvData = malloc(a_i32MemorySize);

	return pvData;
}

/**
 *
 * DESCRIPTION
 * The OSAL API de-allocates assigned memory pointed by the argument.
 *
 * @param pPointer [in] void* Pointer to the memory.
 *
 * @return None
 */
void OSAL_Free(void *pvPointer)
{
    // Check for NULL pointer free
    if(NULL == pvPointer)
    {
        return;
    }

    free(pvPointer);
	pvPointer = NULL;

}

/**
 *
 * DESCRIPTION
 * The OSAL API creates a thread with the specified thread parameters
 * for various OS.
 *
 * @param pThreadParam [in] thread_Create_t* Pointer to structure holding
 *                  		thread creation parameters.
 *
 * @return [out] Thread_H thread id of newly created thread;
 * 				 -1 in case of NULL input parameters structure or error
 *
 */
Thread_H Osal_Thread_Create(thread_Create_t *pThreadParam)
{
    pthread_attr_t attr;
    int retcode;

    // null check
    if(NULL == pThreadParam)
    {
    	printf("NULL pointer received in Osal_Thread_Create\n");
    	return -1;
    }
    // Set up thread attributes
    pthread_attr_init(&attr);

    // set stack size for thread
    if(pThreadParam->dwStackSize > 0)
    {
    	retcode = pthread_attr_setstacksize(&attr, pThreadParam->dwStackSize);
    	if (retcode != 0)
    	{
    		perror("pthread_attr_setstacksize failed :: ");
    	}
    }

    // Start the thread
    retcode = pthread_create(pThreadParam->lpThreadId, &attr, pThreadParam->lpStartAddress,
                            pThreadParam->lpParameter);

    // clear up assigned attributes
    pthread_attr_destroy(&attr);

    if (retcode == 0)
    {
        return (*(pThreadParam->lpThreadId));
    }
    else
    {
    	perror("pthread_create failed::");
    	return -1;
    }
}

/**
 *
 * DESCRIPTION
 * The OSAL API terminates thread specified by thread handle/ thread ID.
 *
 * @param pThreadTerminate [in] Thread_H Pointer to structure holding
 *                  			parameters of the thread to terminate.
 * @return true if thread terminates successfully;
 * 		   false if fails to terminate thread
 *
 */
bool Osal_Thread_Terminate(Thread_H pThreadTerminate)
{
    // Cancel thread
    pthread_cancel(pThreadTerminate);

    // Wait for thread to stop and reclaim thread resources
    if (0 == pthread_join(pThreadTerminate, NULL ) )
    {
        return true;
    }
    else
    {
    	return false;
    }
}

/**
 *
 * DESCRIPTION
 * The OSAL API copies a message in Linux message queue.
 *
 * @param pstPostThreadMsg [in] Post_Thread_Msg_t* Pointer to structure to be copied
 * 								in Linux message queue.
 *
 * @return true if function succeeds to add message in Linux message queue;
 * 		   false if function fails to add message in Linux message queue
 *
 */
bool OSAL_Post_Message(Post_Thread_Msg_t *pstPostThreadMsg)
{
	int iStatus = 0;
	Linux_Msg_t stMsgData;
	size_t MsgSize = 0;

	stMsgData.mtype = pstPostThreadMsg->MsgType;
	stMsgData.wParam = pstPostThreadMsg->wParam;
	stMsgData.lParam = pstPostThreadMsg->lParam;

	MsgSize = sizeof(Linux_Msg_t) - sizeof(long);

	iStatus = msgsnd( pstPostThreadMsg->idThread, &stMsgData, MsgSize, 0);

	if(iStatus == 0)
	{
		return true;
	}
	else
	{
		// addressed review comment
		perror("Error in msgsnd:: ");
		return false;
	}
}


/**
 *
 * DESCRIPTION
 * The OSAL API retrieves the message from Linux message queue. This function blocks/ waits till
 * either a message is received or an error occurs. It retrieves message of the specified
 * message id and stores message in pstQueueMsg.
 *
 * @param pstQueueMsg [out] Linux_Msg_t* Pointer to structure where message is to be stored.
 * @param msqid 	  [in]  int Linux message id to be retrieved.
 *
 * @return true if function succeeds in retrieving the message;
 * 		   false if any error occurs while retrieving the message
 *
 */
bool OSAL_Get_Message(Linux_Msg_t *pstQueueMsg, int   msqid)
{
	int32_t i32RetVal = 0;

	size_t MsgSize = 0;

	MsgSize = sizeof(Linux_Msg_t) - sizeof(long);

	// Wait Till Message received or Error other than Signal interrupt
	do
	{
		i32RetVal = msgrcv(msqid, pstQueueMsg, MsgSize, MAX_RECV_PRIORITY, 0);
		if(i32RetVal < 0 && errno == EINTR)
		{
			// addressed review comment
			perror("msgrcv error :; ");
			continue;
		}
		else
		{
			// success
			break;
		}

	}while(i32RetVal > 0);

	// Returns Number of Bytes received or Error received
	return( i32RetVal);
}

/**
 * DESCRIPTION
 * The OSAL API retrieves the message from Linux message queue, without blocking/ waiting
 * for an error or a message. It retrieves message of the specified message id and stores
 * message in pstQueueMsg.
 *
 * @param pstQueueMsg [out] Linux_Msg_t* Pointer to struct where data is copied.
 * @param msqid 	  [in] int Linux message id to be retrieved.
 *
 * @return true if function succeeds in retrieving the message;
 * 		   false if any error occurs while retrieving the message
 *
 */
int32_t OSAL_Get_NonBlocking_Message(Linux_Msg_t *pstQueueMsg, int   msqid)
{
	int32_t i32RetVal = 0;

	size_t MsgSize = 0;

	MsgSize = sizeof(Linux_Msg_t) - sizeof(long);

    // Wait Till Message received or Error other than Signal interrupt
    i32RetVal = msgrcv(msqid, pstQueueMsg, MsgSize, MAX_RECV_PRIORITY, MSG_NOERROR | IPC_NOWAIT);
    if(errno == ENOMSG && (-1 == i32RetVal) && EINTR == errno)
    {
    	// addressed review comment
    	perror("failed to recv msg from queue ::");
    	i32RetVal = -1;
    }

    // Returns Number of Bytes received or Error received
	return( i32RetVal);
}


/**
 *
 * DESCRIPTION
 * This OSAL API initializes the Linux message queue to store requests sent to
 * the Modbus slave device.
 *
 * @param Nothing
 *
 * @return [out] int32_t Linux message queue id if function succeeds
 * 				   		-1 in case if function fails to create a message queue
 *
 */
int32_t OSAL_Init_Message_Queue()
{
    int msqid = 0;

    msqid = msgget(IPC_PRIVATE, (IPC_CREAT | IPC_EXCL | 0666));

    if(msqid < 0)
    {
    	perror("failed to create message queue:: ");
    	return -1;
    }

    return msqid;
}

/**
 *
 * DESCRIPTION
 * This OSAL API deletes message queue with specified message queue id.
 *
 * @param MsgQId [in] int Message queue id to delete
 *
 * @return [out] bool true if function deletes the message queue successfully;
 * 					  false if function fails to delete the message queue
 *
 */
bool OSAL_Delete_Message_Queue(int MsgQId)
{
	int iStatus = 0;
	iStatus = msgctl(MsgQId, IPC_RMID, NULL);

    if(iStatus < 0)
    {
    	perror("failed to delete message queue:: ");
    	return false;
    }
    return true;
}

/**
 *
 * DESCRIPTION
 * The OSAL API creates a mutex for various OS.
 *
 * @param None
 * @return [out] Mutex_H Returns Handle of the newly created mutex if function succeeds;
 * 				NULL if function fails
 *
 */
Mutex_H Osal_Mutex(void)
{
	Mutex_H pstTpmPtr = NULL;

	// Assign memory to mutex handle
	pstTpmPtr = (Mutex_H)OSAL_Malloc(sizeof(pthread_mutex_t));

	if (NULL == pstTpmPtr)
	{
		printf("failed to create mutex..\n");
		return NULL;
	}

	int iRetVal = pthread_mutex_init(pstTpmPtr, NULL);
	if(EAGAIN == iRetVal || ENOMEM == iRetVal || EPERM == iRetVal)
	{
		// destroy the mutex
		perror("mutex creation failed :: ");
		OSAL_Free(pstTpmPtr);
		return NULL;
	}
	else if (0 == iRetVal)
	{
		// SUCCESS
	}
	else
	{
		// other cases free up the memory
		perror("mutex creation failed:: ");
		OSAL_Free(pstTpmPtr);
		return NULL;
	}

	return pstTpmPtr;
}

/**
 *
 * DESCRIPTION
 * The OSAL API waits to lock the mutex specified by the mutex handle.
 *
 * @param pMtxHandle [in] Mutex_H handle to mutex which needs to be locaked
 *
 * @return [out] int32_t 0 if the function succeeds;
 * 						 non-zero if function fails.
 *
 */
int32_t Osal_Wait_Mutex(Mutex_H pMtxHandle)
{
	if (NULL == pMtxHandle)
	{
		// error
		printf("Invalid mutex handle received while acquiring the mutex\n");
		return -1;
	}
	else if(!(pthread_mutex_lock(pMtxHandle)))
	{
		// success
    	return 0;
	}
    else
    {
    	// return pthread error code
    	// addressed review comment
    	perror("Failed to lock mutex, error ::");
    	return errno;
    }
}

/**
 *
 * DESCRIPTION
 * The OSAL API release the mutex specified by the mutex handle.
 *
 * @param pMtxHandle [in] Mutex_H handle to mutex which needs to be locaked
 *
 * @return [out] int32_t 0 if the function succeeds;
 * 						 non-zero if function fails.
 *
 */
int32_t Osal_Release_Mutex(Mutex_H pMtxHandle)
{
	if(NULL == pMtxHandle)
	{
		// error
		printf("Invalid mutex handle received while releasing the mutex\n");
		return -1;
	}
	else if(!(pthread_mutex_unlock(pMtxHandle)))
	{
		// success
    	return 0;
	}
	else
	{
    	// return pthread error code
    	// addressed review comment
    	perror("Failed to unlock mutex, error ::");
    	return errno;
	}
}

/**
 *
 * DESCRIPTION
 * The OSAL API destroys the mutex specified by the mutex handle.
 *
 * @param pMtxHandle [in] Mutex_H handle to mutex which needs to be locaked
 *
 * @return [out] int32_t 0 if the function succeeds;
 * 						 non-zero if function fails.
 *
 */
int32_t Osal_Close_Mutex( Mutex_H pMtxHandle)
{
   if (NULL == pMtxHandle)
   {
      return -1;
   }

   if (pthread_mutex_destroy(pMtxHandle) == 0)
   {
	   if(!pMtxHandle)
	   {
		   OSAL_Free(pMtxHandle);
	   }
       return 0;
   }
   else
   {
		// return pthread error code
		// addressed review comment
		perror("Failed to destroy mutex, error ::");
		return errno;
   }
}

