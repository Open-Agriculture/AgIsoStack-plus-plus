//================================================================================================
/// @file stm32l4x_hal.cpp
///
/// @brief An interface for using CAN peripherals on STM32L4x devices.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "isobus/hardware_integration/stm32l4x_hal.hpp"

#include "isobus/isobus/can_stack_logger.hpp"

namespace isobus
{
#ifdef USE_CMSIS_RTOS2_THREADING
	osSemaphoreId_t STM32L4xHALPlugin::can1RxSemaphore = nullptr;
#ifdef CAN2
	osSemaphoreId_t STM32L4xHALPlugin::can2RxSemaphore = nullptr;
#endif
#endif

	CAN_HandleTypeDef STM32L4xHALPlugin::can1Handle; ///< A copy of the handle for CAN 1, which is static so that we can access it in an interrupt

#ifdef CAN2
	CAN_HandleTypeDef STM32L4xHALPlugin::can2Handle;
#endif

	LockFreeQueue<CANMessageFrame> STM32L4xHALPlugin::can1Queue(DEFAULT_QUEUE_SIZE);
#ifdef CAN2
	LockFreeQueue<CANMessageFrame> STM32L4xHALPlugin::can2Queue(DEFAULT_QUEUE_SIZE);
#endif

	STM32L4xHALPlugin::STM32L4xHALPlugin(const CAN_HandleTypeDef &handle)
	{
		if (CAN1 == handle.Instance)
		{
			can1Handle = handle;
		}
#ifdef CAN2
		else
		{
			can2Handle = handle;
			channel = PeripheralChannel::Channel2;
		}
#endif
	}

	bool STM32L4xHALPlugin::get_is_valid() const
	{
		return isOpen;
	}

	void STM32L4xHALPlugin::close()
	{
		HAL_CAN_Stop(&get_channel());

#if defined(CAN2)
		if (canHandle.Instance == CAN2)
		{
			HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
			HAL_NVIC_DisableIRQ(CAN2_RX1_IRQn);
			HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
		}
		else
		{
			HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
			HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
			HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
		}
#else
		HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
		HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
		HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
#endif
	}

	void STM32L4xHALPlugin::open()
	{
#if defined(CAN2)
		if (get_channel().Instance == CAN2)
		{
			HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
			HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
			HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
		}
		else
		{
			HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
			HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
			HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
		}
#else

#ifdef osCMSIS_FreeRTOS // FreeRTOS is picky about interrupts that are called from ISRs...
		HAL_NVIC_SetPriorityGrouping(0);
#endif
		HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
		HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
		HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
#endif
		const CAN_FilterTypeDef filter = {
			0,
			0,
			0,
			0,
			CAN_FILTER_FIFO0,
			1,
			CAN_FILTERMODE_IDMASK,
			CAN_FILTERSCALE_32BIT,
			ENABLE,
			0
		};

		HAL_CAN_ConfigFilter(&get_channel(), &filter);
		isOpen = (HAL_OK == HAL_CAN_Start(&get_channel()));

#if defined(CAN2)
		if (!isOpen)
		{
			if (PeripheralChannel::Channel2 == channel)
			{
				LOG_CRITICAL("[CAN2]: Failed to start CAN channel!");
			}
			else
			{
				LOG_CRITICAL("[CAN1]: Failed to start CAN channel!");
			}
		}
#else
		if (!isOpen)
		{
			LOG_CRITICAL("[CAN1]: Failed to start CAN channel!");
		}
#endif

		if (isOpen)
		{
#if defined(CAN2)
			if (PeripheralChannel::Channel2 == channel)
			{
				if (nullptr == can2RxSemaphore)
				{
					const osSemaphoreAttr_t attributes = { nullptr, 0, nullptr, 0 };
					can2RxSemaphore = osSemaphoreNew(1, 0, &attributes);

					if (nullptr == can2RxSemaphore)
					{
						LOG_CRITICAL("[CAN2]: Failed to create rx semaphore. Make sure you have enough global memory pool space assigned to your RTOS.");
					}
				}
			}
			else
			{
				if (nullptr == can1RxSemaphore)
				{
					const osSemaphoreAttr_t attributes = { nullptr, 0, nullptr, 0 };
					can1RxSemaphore = osSemaphoreNew(1, 0, &attributes);

					if (nullptr == rxSemaphore)
					{
						LOG_CRITICAL("[CAN1]: Failed to create rx semaphore. Make sure you have enough global memory pool space assigned to your RTOS.");
					}
				}
			}
#else
			if (nullptr == can1RxSemaphore)
			{
				const osSemaphoreAttr_t attributes = { nullptr, 0, nullptr, 0 };
				can1RxSemaphore = osSemaphoreNew(1, 0, &attributes);

				if (nullptr == can1RxSemaphore)
				{
					LOG_CRITICAL("[CAN1]: Failed to create rx semaphore. Make sure you have enough global memory pool space assigned to your RTOS.");
				}
			}
#endif
			HAL_CAN_ActivateNotification(&get_channel(), CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY);
		}
	}

	bool STM32L4xHALPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		if (isOpen)
		{
			retVal = getCANMessage(canFrame);
			if (!retVal)
			{
#ifdef USE_CMSIS_RTOS2_THREADING
				osSemaphoreId_t targetPeripheralHandle = can1RxSemaphore;

#ifdef CAN2
				if (PeripheralChannel::Channel2 == channel)
				{
					targetPeripheralHandle = can2RxSemaphore;
				}
#endif

				// Wait for a frame to arrive
				if (osOK == osSemaphoreAcquire(targetPeripheralHandle, 100))
				{
					retVal = getCANMessage(canFrame);
				}
				else
				{
					// No frames... come back later
				}
#else
				retVal = false; // Not really supported...
#endif
			}
		}
		return retVal;
	}

	bool STM32L4xHALPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		if (isOpen)
		{
			CAN_TxHeaderTypeDef txHeader = { 0, canFrame.identifier, CAN_ID_EXT, CAN_RTR_DATA, canFrame.dataLength, DISABLE };
			std::uint32_t mailbox = 0;

			if (HAL_OK == HAL_CAN_AddTxMessage(&get_channel(), &txHeader, &canFrame.data[0], &mailbox))
			{
				// We'll assume the peripheral sends it out. If it doesn't, subsequent messages > mailbox count may fail which indicates a config or bus issue.
				retVal = true;
			}
		}
		return retVal;
	}

	void STM32L4xHALPlugin::can1_rx_callback()
	{
		CAN_RxHeaderTypeDef rxHeader;
		isobus::CANMessageFrame canFrame;

		while (HAL_OK == HAL_CAN_GetRxMessage(&get_can1_handle(), CAN_RX_FIFO0, &rxHeader, &canFrame.data[0]))
		{
			canFrame.dataLength = rxHeader.DLC;

			if (CAN_ID_EXT == rxHeader.IDE)
			{
				canFrame.identifier = rxHeader.ExtId;
			}
			else
			{
				canFrame.identifier = rxHeader.StdId;
			}
			canFrame.isExtendedFrame = (CAN_ID_EXT == rxHeader.IDE);

			can1Queue.push(canFrame);
		}
		if (nullptr != can1RxSemaphore)
		{
			osSemaphoreRelease(can1RxSemaphore);
		}
	}

#ifdef CAN2
	void STM32L4xHALPlugin::can2_rx_callback()
	{
		CAN_RxHeaderTypeDef rxHeader;
		isobus::CANMessageFrame canFrame;

		while (osOK == HAL_CAN_GetRxMessage(&get_can2_handle(), CAN_RX_FIFO0, &rxHeader, &canFrame.data[0]))
		{
			canFrame.dataLength = rxHeader.DLC;

			if (CAN_ID_EXT == rxHeader.IDE)
			{
				canFrame.identifier = rxHeader.ExtId;
			}
			else
			{
				canFrame.identifier = rxHeader.StdId;
			}
			canFrame.isExtendedFrame = (CAN_ID_EXT == rxHeader.IDE);

			can2Queue.push(canFrame);
		}
		if (nullptr != can2RxSemaphore)
		{
			osSemaphoreRelease(can2RxSemaphore);
		}
	}
#endif

	CAN_HandleTypeDef &STM32L4xHALPlugin::get_can1_handle()
	{
		return can1Handle;
	}

#ifdef CAN2
	CAN_HandleTypeDef &STM32L4xHALPlugin::get_can2_handle()
	{
		return can2Handle;
	}
#endif

	bool STM32L4xHALPlugin::getCANMessage(isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		if (can1Queue.peek(canFrame))
		{
			can1Queue.pop();
			retVal = true;
		}
		return retVal;
	}

	CAN_HandleTypeDef &STM32L4xHALPlugin::get_channel()
	{
#ifdef CAN2
		if (PeripheralChannel::Channel2 == channel)
		{
			return get_can2_handle();
		}
		return get_can1_handle();
#else
		return get_can1_handle();
#endif
	}
} // namespace isobus

extern void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if (CAN1 == hcan->Instance)
	{
		isobus::STM32L4xHALPlugin::can1_rx_callback();
	}
#if defined(CAN2)
	else
	{
		isobus::STM32L4xHALPlugin::can2_rx_callback();
	}
#endif
}
