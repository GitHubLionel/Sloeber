/**
 * A simple class to easily manage a list of tasks
 *
 * You have a global instance of the class : TaskList
 *
 * So, in the end of the setup, you just need to have for example :
 *
 * TaskList.AddTask({condCreate, "Task_Name", 1024, 5, 2000, CoreAny, Task_Function});
 * TaskList.Create();
 *
 * See TaskData_t struct for the meaning of each data parameters of the task
 * That's all :)
 *
 * The code of the function of the task look like that :
 *
 void Task_Function(void *parameter)
 {
	 BEGIN_TASK_CODE("Task_Name");
	 for (EVER)
	 {
		 ... your code

		 END_TASK_CODE(false); // False to not suspend the task
	 }
 }
 *
 * When the function is basic, like for example just read Dallas sensor, you can use
 * the 2 macros BEGIN_TASK_CODE and END_TASK_CODE to simplify the code.
 *
 * A special task called "Memory" is created when the define RUN_TASK_MEMORY == true.
 * This special task is used to get the remaining memory of all the tasks (for debug purpose).
 * This task is running every 10 s at low priority (2).
 *
 * When you create all the tasks, you can add another special task to monitore the cpu load.
 * This task evaluate the cpu load. Just call GetIdleStr() function to print the Idle of core 1 and core 2
 *
 *********************************************************************
 * TIPS : The upshot: if the operation you want to protect is simply a read-modify-write, use a
 * critical section. If the operation you want to protect takes longer (say, sending bytes into an UART), use a mutex.
 * Critical section:
 * 	portMUX_TYPE crit_lock = portMUX_INITIALIZER_UNLOCKED;
 * 	taskENTER_CRITICAL(&crit_lock);
 * 	... code
 * 	taskEXIT_CRITICAL(&crit_lock);
 *
 * 	Mutex section:
 * 	SemaphoreHandle_t sema_lock = xSemaphoreCreateBinary();
 * 	xSemaphoreTake(sema_lock, portMAX_DELAY);
 * 	... code
 * 	xSemaphoreGive(sema_lock);
 *********************************************************************
 * For the WatchDog:
 * To increase the timeout period :
 * - in sdkconfig.h, increase CONFIG_ESP_TASK_WDT_TIMEOUT_S
 * or
 * - use esp_task_wdt_init() function
 */
#pragma once
#ifndef TASKS_UTILS
#define TASKS_UTILS

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include <Arduino.h>
#include <vector>

// To see the remaining memory of each task. Use it for debug
#ifndef RUN_TASK_MEMORY
#define RUN_TASK_MEMORY	false
#endif

// The stack size for the task "Memory". You may increase this value if you have a lot of tasks
#define TASK_MEMORY_STACK	4096

// For infinite loop : for (EVER)
#define EVER ;;

// Some defines to simplify task code.
// This define create a pointer to the struct TaskData_t of the task that can used in the code.
// BEWARE : no verification is done on the pointer !
#define BEGIN_TASK_CODE(name)	TaskData_t *td = TaskList.GetTaskByName(name); \
		int sleep = pdMS_TO_TICKS(td->Sleep_ms);

#if RUN_TASK_MEMORY == true
#define END_TASK_CODE(suspend)	{ td->Memory = uxTaskGetStackHighWaterMark(NULL); \
		taskYIELD(); \
		if ((suspend) && !td->IsSuspended) \
			{ td->IsSuspended = true; vTaskSuspend(td->Handle);} \
	  else \
			if (sleep != 0) vTaskDelay(sleep);}
#else
#define END_TASK_CODE(suspend)	{ taskYIELD(); \
		if ((suspend) && !td->IsSuspended) \
			{ td->IsSuspended = true; vTaskSuspend(td->Handle);} \
		else \
			if (sleep != 0) vTaskDelay(sleep);}
#endif

// The same couple of define to use with DelayUntil
#define BEGIN_TASK_CODE_UNTIL(name)	TaskData_t *td = TaskList.GetTaskByName(name); \
		int sleep = pdMS_TO_TICKS(td->Sleep_ms); \
		TickType_t xLastWakeTime;

#if RUN_TASK_MEMORY == true
#define END_TASK_CODE_UNTIL(suspend)	{ td->Memory = uxTaskGetStackHighWaterMark(NULL); \
		if ((suspend) && !td->IsSuspended) \
			{ td->IsSuspended = true; vTaskSuspend(td->Handle);} \
		else \
		  xTaskDelayUntil(&xLastWakeTime, sleep);}
#else
#define END_TASK_CODE_UNTIL(suspend)	{ \
		if ((suspend) && !td->IsSuspended) \
			{ td->IsSuspended = true; vTaskSuspend(td->Handle);} \
		else \
			xTaskDelayUntil(&xLastWakeTime, sleep);}
#endif

// Core used by a task
typedef enum
{
	Core0 = 0,
	Core1 = 1,
	CoreAny
} Task_Core;

// The initial condition when we call Create() function for the task
typedef enum
{
	condNotCreate = 0,  // Not create the task, should be created later with CreateTask() function
	condCreate,         // Create the task and start it immediatly
	condSuspended       // Create the task suspended. Call ResumeTask() to start the task
} Task_Condition;

/**
 * Task structure
 * All parameters are required except Param, Memory, UserParam, Handle and IsSuspended.
 * Condition is a parameter only used in the main Create() function. If 'condNotCreate' then the task
 * will not be created and could be created separatly with CreateTask().
 * Handle should be always initialized to NULL because it will be filled when task is created.
 * UserParam can be used to pass information to the task when it is running.
 * Memory is only used when the "Memory" task is defined : define RUN_TASK_MEMORY == true
 */
typedef struct
{
	public:
		Task_Condition Condition = condNotCreate; // Initial condition of the task for the Create() function
		char Name[configMAX_TASK_NAME_LEN + 1];   // Task name
		configSTACK_DEPTH_TYPE StackSize;         // Task stack size
		UBaseType_t Priority;                     // Task priority : 0 lower to higher configMAX_PRIORITIES - 1
		int Sleep_ms;                             // Task sleep delay in ms
		Task_Core Core;												  	// Core where is running the task
		TaskFunction_t TaskCode;							  	// The code of the task
		void *Param = NULL;										  	// The parameter passed to the code function
		int Memory = 0;                           // Task stack staying memory (used by the "Memory" task)
		void *UserParam = NULL;						  		  // A parameter that can be used in the code function
		TaskHandle_t Handle = NULL;							  // The handle of the task
		bool IsSuspended = false;                 // Is the task suspended ?
} TaskData_t;

/**
 * The task list class
 * Set RUN_TASK_MEMORY to true to have Memory task running
 * A global instance of TaskList_c is automaticaly created named TaskList
 */
class TaskList_c
{
	public:
		TaskList_c(bool with_memory = false);

		bool Create(bool with_idle_task);
		void AddTask(const TaskData_t task);

		void DeleteTask(const String &name);
		bool CreateTask(const String &name, bool forceDelete = false, void *param = NULL);
		void SuspendTask(const String &name);
		void ResumeTask(const String &name);
		void ChangeSleepTask(const String &name, int sleep_ms, bool reload = false);
		bool IsTaskRunning(const String &name);
		bool IsTaskSuspended(const String &name);

		TaskData_t* GetTaskByName(const String &name);
		int GetTaskSleep(const String &name, bool to_ticks = true);
		TaskHandle_t GetTaskHandle(const String &name);

		void SuspendAllTask(void);

		String GetIdleStr(void) const;
		String GetMemoryStr(void) const;

		void InfoTask(void);

		// Put in public to have access with the global instance
		void Task_Memory_code(void *parameter);

	protected:
		std::vector<TaskData_t> Tasks;
		TaskData_t TaskMemory;
		bool task_created;
};

/**
 * A global instance of TaskList
 */
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_TASKLIST)
#define TASKLIST_DEFINED
extern TaskList_c TaskList;
#endif

#endif
