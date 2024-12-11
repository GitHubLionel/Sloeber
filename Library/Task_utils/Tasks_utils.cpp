#include "Tasks_utils.h"
//#include "FreeRTOSConfig.h"

/**
 * A global instance of TaskList
 */
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_TASKLIST)
TaskList_c TaskList = TaskList_c(RUN_TASK_MEMORY);
#else
#warning "Global instance of TaskList_c not created: Memory task not available"
#define NO_MEMORY_TASK
#endif

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const String mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

// Function to test cpu load
// see https://esp32.com/viewtopic.php?t=3536
void Task_Idle0_code(void *parameter);
void Task_Idle1_code(void *parameter);
void Task_IdleSecond_code(void *parameter);

// Memory task
void Task_Memory_code_local(void *parameter)
{
	TaskList.Task_Memory_code(parameter);
}

const TaskData_t TaskZero = {false, "zero", 1024, 0, 1000, CoreAny, NULL};
const TaskData_t TaskIdle0 = {true, "TK_Idle0", 1024, tskIDLE_PRIORITY, 10, Core0, Task_Idle0_code};
const TaskData_t TaskIdle1 = {true, "TK_Idle1", 1024, tskIDLE_PRIORITY, 10, Core1, Task_Idle1_code};
const TaskData_t TaskIdleSecond = {true, "IdleSecond", 4096, 10, 1000, CoreAny, Task_IdleSecond_code};

static portMUX_TYPE Idlelock = portMUX_INITIALIZER_UNLOCKED;
static uint32_t count_Idle0 = 0;
static uint32_t count_Idle1 = 0;
static String Idle_str = "";
static String Memory_str = "";

/**
 * TaskList_c constructor
 * with_memory: add memory task (default false)
 * Global instance TaskList of TaskList_c use RUN_TASK_MEMORY as parameter
 */
TaskList_c::TaskList_c(bool with_memory)
{
	if (with_memory)
	{
#if (RUN_TASK_MEMORY == true) && !defined(NO_MEMORY_TASK)
		TaskMemory = {true, "Memory_Task", TASK_MEMORY_STACK, 2, 10000, CoreAny, Task_Memory_code_local};
		AddTask(TaskMemory);
#endif
	}
	task_created = false;
}

/**
 * Create all the tasks.
 * If param with_idle_task == true (default false) then we also create tasks to measure idle percent of each core.
 * Call TaskList.GetIdleStr() to print the percent of each core.
 * Typically, this method should be call at the end of setup and only once
 * Return true if all tasks are created
 */
bool TaskList_c::Create(bool with_idle_task)
{
	if (task_created)
		return false;

	bool success = true;
	BaseType_t xReturned = pdPASS;

	for (int i = 0; i < Tasks.size(); i++)
	{
		if (Tasks[i].Condition)
		{
			if (Tasks[i].Core == CoreAny)
			{
				xReturned = xTaskCreate(Tasks[i].TaskCode, // Task code
						Tasks[i].Name,             // Task name
						Tasks[i].StackSize,        // Stack size (bytes)
						(void*) Tasks[i].Param,    // Parameter
						Tasks[i].Priority,         // Task priority
						&Tasks[i].Handle           // Task handle
						);
			}
			else
			{
				xReturned = xTaskCreatePinnedToCore(Tasks[i].TaskCode, // Task code
						Tasks[i].Name,             // Task name
						Tasks[i].StackSize,        // Stack size (bytes)
						(void*) Tasks[i].Param,    // Parameter
						Tasks[i].Priority,         // Task priority
						&Tasks[i].Handle,          // Task handle
						Tasks[i].Core							 // Task core
						);
			}
		}
		success = success && (xReturned == pdPASS);
	}

	if (with_idle_task)
	{
		xTaskCreatePinnedToCore(TaskIdle0.TaskCode, // Task code
				TaskIdle0.Name,             // Task name
				TaskIdle0.StackSize,        // Stack size (bytes)
				NULL,                 		  // Parameter
				TaskIdle0.Priority,         // Task priority
				NULL,                       // Task handle
				TaskIdle0.Core							// Task core
				);

		xTaskCreatePinnedToCore(TaskIdle1.TaskCode, // Task code
				TaskIdle1.Name,             // Task name
				TaskIdle1.StackSize,        // Stack size (bytes)
				NULL,                 		  // Parameter
				TaskIdle1.Priority,         // Task priority
				NULL,                       // Task handle
				TaskIdle1.Core							// Task core
				);

		xTaskCreate(TaskIdleSecond.TaskCode, // Task code
				TaskIdleSecond.Name,             // Task name
				TaskIdleSecond.StackSize,        // Stack size (bytes)
				NULL,                 		       // Parameter
				TaskIdleSecond.Priority,         // Task priority
				NULL                             // Task handle
				);
	}
	task_created = true;

	return success;
}

/**
 * Add task to the task list
 * BEWARE: if the task can self delete, then do not forget to initialize the Handle of the task data to NULL
 * in the code of the function.
 * Example:
 *
 void Task_Function(void *parameter)
 {
	 BEGIN_TASK_CODE("Task_Name");
	 for (EVER)
	 {
		 ... code
		 if (condition)
		 {
			 vTaskDelete(td->Handle);
			 td->Handle = NULL;
		 }
		 else
		 	 END_TASK_CODE();
	 }
 }
 */
void TaskList_c::AddTask(const TaskData_t task)
{
	Tasks.push_back(task);
}

/**
 * Delete a task
 * The task is only deleded to the process but stay in the task list
 * It is probably better to use UserParam to send a message to the task to self delete
 */
void TaskList_c::DeleteTask(const String &name)
{
	TaskData_t *td = GetTaskByName(name);
	if ((td != NULL) && (td->Handle != NULL))
	{
		vTaskSuspend(td->Handle);
		vTaskDelete(td->Handle);
		td->Handle = NULL;
	}
}

/**
 * Create (or recreate) a task who is in the task list
 * If the task already exist do nothing except if force = true (default false)
 * Pass param if no null (default NULL)
 */
bool TaskList_c::CreateTask(const String &name, bool force, void *param)
{
	BaseType_t xReturned = pdFAIL;
	TaskData_t *td = GetTaskByName(name);
	if (td != NULL)
	{
		// If task exist and force = true, then delete the task
		if ((td->Handle != NULL) && force)
		{
			vTaskSuspend(td->Handle);
			vTaskDelete(td->Handle);
			td->Handle = NULL;
		}

		// Create the task
		if (td->Handle == NULL)
		{
			// Replace the parameter pass to the function
			if (param != NULL)
				td->Param = param;

			if (td->Core == CoreAny)
			{
				xReturned = xTaskCreate(td->TaskCode, // Task code
						td->Name,             // Task name
						td->StackSize,        // Stack size (bytes)
						(void*) td->Param,    // Parameter
						td->Priority,         // Task priority
						&td->Handle           // Task handle
						);
			}
			else
			{
				xReturned = xTaskCreatePinnedToCore(td->TaskCode, // Task code
						td->Name,             // Task name
						td->StackSize,        // Stack size (bytes)
						(void*) td->Param,    // Parameter
						td->Priority,         // Task priority
						&td->Handle,          // Task handle
						td->Core							// Task core
						);
			}
		}
	}
	return (xReturned == pdPASS);
}

/**
 * Suspend a task
 */
void TaskList_c::SuspendTask(const String &name)
{
	TaskData_t *td = GetTaskByName(name);
	if (td != NULL)
	{
		vTaskSuspend(td->Handle);
	}
}

/**
 * Resume a task
 */
void TaskList_c::ResumeTask(const String &name)
{
	TaskData_t *td = GetTaskByName(name);
	if (td != NULL)
	{
		vTaskResume(td->Handle);
	}
}

/**
 * Change the sleep time of a task and reload (default false)
 */
void TaskList_c::ChangeSleepTask(const String &name, int sleep_ms, bool reload)
{
	TaskData_t *td = GetTaskByName(name);
	if (td != NULL)
	{
		td->Sleep_ms = sleep_ms;
		if (reload)
			CreateTask(name, true);
	}
}

/**
 * Return true if the task exist and is running
 */
bool TaskList_c::IsTaskRunning(const String &name)
{
	TaskData_t *td = GetTaskByName(name);
	return ((td != NULL) && (td->Handle != NULL));
}

/**
 * Get a task from the task list by it's name
 */
TaskData_t* TaskList_c::GetTaskByName(const String &name)
{
	for (int i = 0; i < Tasks.size(); i++)
		if (strcmp(Tasks[i].Name, name.c_str()) == 0)
			return &Tasks[i];
	return NULL;    // Not found
}

/**
 * Get the sleeping time of a task
 */
int TaskList_c::GetTaskSleep(const String &name, bool to_ticks)
{
	TaskData_t *td = GetTaskByName(name);
	if (td != NULL)
	{
		if (to_ticks)
			return pdMS_TO_TICKS(td->Sleep_ms);
		else
			return td->Sleep_ms;
	}
	else
		return -1;    // Not found
}

/**
 * Get the handle of a task
 */
TaskHandle_t TaskList_c::GetTaskHandle(const String &name)
{
	TaskData_t *td = GetTaskByName(name);
	if (td != NULL)
	{
		return td->Handle;
	}
	else
		return NULL;
}

/**
 * Print info (priority, sleep, n° core, size) of each tasks
 */
void TaskList_c::InfoTask(void)
{
	String info = "--- Task Info ---\r\n";
	for (int i = 0; i < Tasks.size(); i++)
	{
		TaskData_t *td = &Tasks[i];
		if (td->Handle != NULL)
			info += String(td->Name) + ":" + " Priority = " + String(td->Priority) +
				", Sleep_ms = " + String(td->Sleep_ms) +
				", Core = " + String(td->Core) +
				", Size = " + String(td->StackSize) + "\r\n";
	}

	print_debug(info, false);
}

// ********************************************************************************
// The purpose of this section is to evaluate the memory used by each task
// ********************************************************************************

/**
 * Print memory used by tasks. For debug purpose.
 */
void TaskList_c::Task_Memory_code(void *parameter)
{
	int sleep = pdMS_TO_TICKS(TaskMemory.Sleep_ms);
	for (EVER)
	{
		Memory_str = "--- Memory free stack ---\r\n";
		for (int i = 0; i < Tasks.size(); i++)
		{
			TaskData_t *td = &Tasks[i];
			if (td->Handle != NULL)
				Memory_str += String(td->Name) + ": " + String(td->Memory) + " / " + String(td->StackSize) + "\r\n";
			taskYIELD();
		}

		print_debug(Memory_str, false);
//		TaskMemory.Memory = uxTaskGetStackHighWaterMark(NULL);
		TaskList.GetTaskByName("Memory_Task")->Memory = uxTaskGetStackHighWaterMark(NULL);

		// Sleep for 10 seconds, avant de refaire une analyse
		vTaskDelay(sleep);
	}
}

String TaskList_c::GetMemoryStr(void) const
{
	return Memory_str;
}

// ********************************************************************************
// The purpose of this section is to evaluate the charge on each core
// ********************************************************************************

String TaskList_c::GetIdleStr(void) const
{
	return "Idle: " + Idle_str;
}

/**
 * Count 10 ms Idle on core 0
 */
void Task_Idle0_code(void *parameter)
{
	int sleep = pdMS_TO_TICKS(TaskIdle0.Sleep_ms);
	for (EVER)
	{
		taskYIELD();
		count_Idle0++;
		vTaskDelay(sleep);
	}
}

/**
 * Count 10 ms Idle on core 1
 */
void Task_Idle1_code(void *parameter)
{
	int sleep = pdMS_TO_TICKS(TaskIdle1.Sleep_ms);
	for (EVER)
	{
		taskYIELD();
		count_Idle1++;
		vTaskDelay(sleep);
	}
}

/**
 * Get the idle counters and reset them
 */
String Get_Idle_Counter(void)
{
	taskENTER_CRITICAL(&Idlelock);
	String count = String(count_Idle0) + " " + String(count_Idle1);
	count_Idle0 = 0;
	count_Idle1 = 0;
	taskEXIT_CRITICAL(&Idlelock);
	return count;
}

/**
 * Task to get the idle counters every second.
 * If the counter of a core equal 100 this mean that this core is idle 100% of the time
 * I use xTaskGetTickCount to have a precise 1 s task
 */
void Task_IdleSecond_code(void *parameter)
{
	int sleep = pdMS_TO_TICKS(TaskIdleSecond.Sleep_ms);
	TickType_t xLastWakeTime;
//	BaseType_t xWasDelayed;

	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	for (EVER)
	{
		// Wait for the next cycle.
		/*xWasDelayed = */xTaskDelayUntil(&xLastWakeTime, sleep);

		// Perform action here. xWasDelayed value can be used to determine
		// whether a deadline was missed if the code here took too long.
		Idle_str = Get_Idle_Counter();
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
